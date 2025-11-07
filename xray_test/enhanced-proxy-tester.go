package main

import (
	"bufio"
	"context"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"math/rand"
	"net"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"regexp"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"syscall"
	"time"

	"golang.org/x/net/proxy"
)

// Enhanced error handling types
type TestError struct {
	Type    string
	Message string
	Cause   error
}

func (e TestError) Error() string {
	if e.Cause != nil {
		return fmt.Sprintf("%s: %s (caused by: %v)", e.Type, e.Message, e.Cause)
	}
	return fmt.Sprintf("%s: %s", e.Type, e.Message)
}

// Enhanced configuration with better defaults
type EnhancedConfig struct {
	XrayPath         string
	MaxWorkers       int
	Timeout          time.Duration
	BatchSize        int
	IncrementalSave  bool
	DataDir          string
	ConfigDir        string
	LogDir           string
	StartPort        int
	EndPort          int

	// New fields for better resource management
	GracefulTimeout  time.Duration
	MaxMemoryMB      int
	EnableMetrics    bool
	MetricsPort      int
	ContextTimeout   time.Duration
}

func NewEnhancedConfig() *EnhancedConfig {
	return &EnhancedConfig{
		XrayPath:         getEnvOrDefault("XRAY_PATH", ""),
		MaxWorkers:       getEnvIntOrDefault("PROXY_MAX_WORKERS", 100), // Reduced from 400
		Timeout:          time.Duration(getEnvIntOrDefault("PROXY_TIMEOUT", 3)) * time.Second,
		BatchSize:        getEnvIntOrDefault("PROXY_BATCH_SIZE", 100), // Reduced from 600
		IncrementalSave:  getEnvBoolOrDefault("PROXY_INCREMENTAL_SAVE", true),
		DataDir:          getEnvOrDefault("PROXY_DATA_DIR", "../data"),
		ConfigDir:        getEnvOrDefault("PROXY_CONFIG_DIR", "../config"),
		LogDir:           getEnvOrDefault("PROXY_LOG_DIR", "../log"),
		StartPort:        getEnvIntOrDefault("PROXY_START_PORT", 10000),
		EndPort:          getEnvIntOrDefault("PROXY_END_PORT", 20000),
		GracefulTimeout:  5 * time.Second,
		MaxMemoryMB:      getEnvIntOrDefault("PROXY_MAX_MEMORY_MB", 1024),
		EnableMetrics:    getEnvBoolOrDefault("PROXY_ENABLE_METRICS", false),
		MetricsPort:      getEnvIntOrDefault("PROXY_METRICS_PORT", 8080),
		ContextTimeout:   10 * time.Second,
	}
}

// Enhanced ProxyTester with proper resource management
type EnhancedProxyTester struct {
	config     *EnhancedConfig
	portMgr    *PortManager
	stats      *Stats
	shutdown   chan struct{}
	wg         sync.WaitGroup
	ctx        context.Context
	cancel     context.CancelFunc

	// Resource tracking
	activeProcesses int64
	memoryUsage     int64
	metrics         *TestMetrics
}

type TestMetrics struct {
	mu                sync.RWMutex
	totalTests        int64
	successfulTests   int64
	failedTests       int64
	avgResponseTime   float64
	memoryUsageMB     int64
	activeConnections int64
	startTime         time.Time
}

func NewTestMetrics() *TestMetrics {
	return &TestMetrics{
		startTime: time.Now(),
	}
}

// Enhanced worker pool with resource limits
type WorkerPool struct {
	maxWorkers     int
	activeWorkers  int64
	taskQueue      chan func()
	resultQueue    chan error
	wg             sync.WaitGroup
	shutdown       chan struct{}
	ctx            context.Context
	cancel         context.CancelFunc
}

func NewWorkerPool(maxWorkers int) *WorkerPool {
	ctx, cancel := context.WithCancel(context.Background())

	return &WorkerPool{
		maxWorkers:  maxWorkers,
		taskQueue:   make(chan func(), maxWorkers*2),
		resultQueue: make(chan error, maxWorkers),
		shutdown:    make(chan struct{}),
		ctx:         ctx,
		cancel:      cancel,
	}
}

func (wp *WorkerPool) Start() {
	for i := 0; i < wp.maxWorkers; i++ {
		go wp.worker()
	}
}

func (wp *WorkerPool) worker() {
	defer wp.wg.Done()

	for {
		select {
		case task := <-wp.taskQueue:
			if task != nil {
				err := wp.safeExecute(task)
				select {
				case wp.resultQueue <- err:
				case <-wp.shutdown:
					return
				}
			}
		case <-wp.shutdown:
			return
		}
	}
}

func (wp *WorkerPool) safeExecute(task func()) (err error) {
	defer func() {
		if r := recover(); r != nil {
			err = fmt.Errorf("panic in worker: %v", r)
		}
	}()

	if task != nil {
		task()
	}
	return nil
}

func (wp *WorkerPool) Submit(task func()) error {
	select {
	case wp.taskQueue <- task:
		return nil
	case <-wp.shutdown:
		return fmt.Errorf("worker pool is shutting down")
	default:
		return fmt.Errorf("worker pool queue is full")
	}
}

func (wp *WorkerPool) Stop() {
	close(wp.shutdown)
	wp.cancel()
	wp.wg.Wait()
	close(wp.taskQueue)
}

func NewEnhancedProxyTester(config *EnhancedConfig) *EnhancedProxyTester {
	ctx, cancel := context.WithCancel(context.Background())

	return &EnhancedProxyTester{
		config:     config,
		portMgr:    NewPortManager(config.StartPort, config.EndPort),
		stats:      NewStats(),
		shutdown:   make(chan struct{}),
		ctx:        ctx,
		cancel:     cancel,
		metrics:    NewTestMetrics(),
	}
}

func (ept *EnhancedProxyTester) Start() error {
	log.Println("Starting Enhanced Proxy Tester...")

	// Ensure directories exist
	if err := ept.ensureDirectories(); err != nil {
		return fmt.Errorf("failed to create directories: %w", err)
	}

	// Setup signal handling for graceful shutdown
	ept.setupSignalHandling()

	// Start metrics server if enabled
	if ept.config.EnableMetrics {
		go ept.startMetricsServer()
	}

	// Start memory monitoring
	go ept.monitorResources()

	log.Printf("Enhanced Proxy Tester started with config: MaxWorkers=%d, Timeout=%v",
		ept.config.MaxWorkers, ept.config.Timeout)

	return nil
}

func (ept *EnhancedProxyTester) Stop() {
	log.Println("Stopping Enhanced Proxy Tester...")

	// Cancel main context
	ept.cancel()

	// Close shutdown channel
	close(ept.shutdown)

	// Wait for all workers to finish
	done := make(chan struct{})
	go func() {
		ept.wg.Wait()
		close(done)
	}()

	// Wait with timeout
	select {
	case <-done:
		log.Println("All workers stopped successfully")
	case <-time.After(ept.config.GracefulTimeout):
		log.Println("Graceful shutdown timeout, forcing termination")
	}

	// Cleanup resources
	ept.portMgr.ReleaseAll()

	log.Println("Enhanced Proxy Tester stopped")
}

func (ept *EnhancedProxyTester) TestConfigsSafe(configs []ProxyConfig, batchID int) ([]*TestResultData, error) {
	if len(configs) == 0 {
		return nil, nil
	}

	log.Printf("Starting safe test of batch %d with %d configurations...", batchID, len(configs))

	// Create worker pool
	workerPool := NewWorkerPool(ept.config.MaxWorkers)
	workerPool.Start()
	defer workerPool.Stop()

	// Create result channel with buffer
	resultChan := make(chan *TestResultData, len(configs))

	// Submit tasks
	for i, config := range configs {
		select {
		case <-ept.ctx.Done():
			return nil, fmt.Errorf("test cancelled: %w", ept.ctx.Err())
		default:
			ept.wg.Add(1)

			task := func(idx int, cfg ProxyConfig) func() {
				return func() {
					defer ept.wg.Done()

					// Check resources before testing
					if !ept.canTest() {
						resultChan <- &TestResultData{
							Config:  cfg,
							Result:  ResultResourceExhausted,
							Message: "Insufficient resources",
						}
						return
					}

					result := ept.testConfigWithResources(&cfg, batchID)
					resultChan <- result
				}
			}(i, config)

			if err := workerPool.Submit(task); err != nil {
				log.Printf("Failed to submit task: %v", err)
				ept.wg.Done()
				continue
			}
		}
	}

	// Wait for all tasks to complete
	ept.wg.Wait()
	close(resultChan)

	// Collect results
	var results []*TestResultData
	for result := range resultChan {
		results = append(results, result)
		ept.updateMetrics(result)
	}

	log.Printf("Completed batch %d: %d/%d successful", batchID,
		ept.countSuccessful(results), len(results))

	return results, nil
}

func (ept *EnhancedProxyTester) testConfigWithResources(config *ProxyConfig, batchID int) *TestResultData {
	start := time.Now()

	// Set timeout context
	ctx, cancel := context.WithTimeout(ept.ctx, ept.config.Timeout)
	defer cancel()

	// Increment active processes
	atomic.AddInt64(&ept.activeProcesses, 1)
	defer atomic.AddInt64(&ept.activeProcesses, -1)

	// Get port
	port, err := ept.portMgr.Acquire()
	if err != nil {
		return &TestResultData{
			Config:  *config,
			Result:  ResultPortConflict,
			Message: fmt.Sprintf("Failed to acquire port: %v", err),
		}
	}
	defer ept.portMgr.Release(port)

	// Create config with test port
	testConfig := *config
	testConfig.Port = port

	// Test with resource monitoring
	result := ept.testSingleConfigResourceAware(ctx, &testConfig)
	result.ResponseTime = time.Since(start)

	return result
}

func (ept *EnhancedProxyTester) testSingleConfigResourceAware(ctx context.Context, config *ProxyConfig) *TestResultData {
	// Check for context cancellation first
	select {
	case <-ctx.Done():
		return &TestResultData{
			Config:  *config,
			Result:  ResultTimeout,
			Message: fmt.Sprintf("Test cancelled: %v", ctx.Err()),
		}
	default:
	}

	// Use the existing test logic but with enhanced error handling
	return ept.testSingleConfig(config)
}

func (ept *EnhancedProxyTester) canTest() bool {
	// Check memory usage
	if ept.config.MaxMemoryMB > 0 {
		var m runtime.MemStats
		runtime.ReadMemStats(&m)
		memoryMB := int64(m.Alloc) / (1024 * 1024)
		if memoryMB > int64(ept.config.MaxMemoryMB) {
			return false
		}
	}

	// Check active processes
	if atomic.LoadInt64(&ept.activeProcesses) >= int64(ept.config.MaxWorkers) {
		return false
	}

	return true
}

func (ept *EnhancedProxyTester) monitorResources() {
	ticker := time.NewTicker(5 * time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ticker.C:
			var m runtime.MemStats
			runtime.ReadMemStats(&m)
			memoryMB := int64(m.Alloc) / (1024 * 1024)
			atomic.StoreInt64(&ept.memoryUsage, memoryMB)

			if ept.config.MaxMemoryMB > 0 && memoryMB > int64(ept.config.MaxMemoryMB) {
				log.Printf("WARNING: Memory usage (%d MB) exceeds limit (%d MB)",
					memoryMB, ept.config.MaxMemoryMB)
			}

		case <-ept.shutdown:
			return
		}
	}
}

func (ept *EnhancedProxyTester) updateMetrics(result *TestResultData) {
	ept.metrics.mu.Lock()
	defer ept.metrics.mu.Unlock()

	atomic.AddInt64(&ept.metrics.totalTests, 1)

	if result.Result == ResultSuccess {
		atomic.AddInt64(&ept.metrics.successfulTests, 1)
	} else {
		atomic.AddInt64(&ept.metrics.failedTests, 1)
	}

	// Update average response time
	if result.ResponseTime > 0 {
		total := atomic.LoadInt64(&ept.metrics.totalTests)
		if total > 0 {
			currentAvg := atomic.LoadFloat64(&ept.metrics.avgResponseTime)
			newAvg := (currentAvg*float64(total-1) + result.ResponseTime.Seconds()) / float64(total)
			atomic.StoreFloat64(&ept.metrics.avgResponseTime, newAvg)
		}
	}
}

func (ept *EnhancedProxyTester) countSuccessful(results []*TestResultData) int {
	count := 0
	for _, result := range results {
		if result.Result == ResultSuccess {
			count++
		}
	}
	return count
}

func (ept *EnhancedProxyTester) ensureDirectories() error {
	dirs := []string{
		ept.config.DataDir,
		ept.config.DataDir + "/working_json",
		ept.config.DataDir + "/working_url",
		ept.config.LogDir,
	}

	for _, dir := range dirs {
		if err := os.MkdirAll(dir, 0755); err != nil {
			return fmt.Errorf("failed to create directory %s: %w", dir, err)
		}
	}

	return nil
}

func (ept *EnhancedProxyTester) setupSignalHandling() {
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)

	go func() {
		select {
		case sig := <-sigChan:
			log.Printf("Received signal %v, initiating graceful shutdown...", sig)
			ept.Stop()
		case <-ept.shutdown:
			return
		}
	}()
}

func (ept *EnhancedProxyTester) startMetricsServer() {
	mux := http.NewServeMux()

	mux.HandleFunc("/metrics", func(w http.ResponseWriter, r *http.Request) {
		ept.serveMetrics(w, r)
	})

	mux.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("OK"))
	})

	server := &http.Server{
		Addr:    fmt.Sprintf(":%d", ept.config.MetricsPort),
		Handler: mux,
	}

	go func() {
		log.Printf("Metrics server starting on port %d", ept.config.MetricsPort)
		if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Printf("Metrics server error: %v", err)
		}
	}()
}

func (ept *EnhancedProxyTester) serveMetrics(w http.ResponseWriter, r *http.Request) {
	ept.metrics.mu.RLock()
	defer ept.metrics.mu.RUnlock()

	metrics := struct {
		TotalTests         int64   `json:"total_tests"`
		SuccessfulTests    int64   `json:"successful_tests"`
		FailedTests        int64   `json:"failed_tests"`
		SuccessRate        float64 `json:"success_rate"`
		AvgResponseTime    float64 `json:"avg_response_time_seconds"`
		MemoryUsageMB      int64   `json:"memory_usage_mb"`
		ActiveProcesses    int64   `json:"active_processes"`
		UptimeSeconds      int64   `json:"uptime_seconds"`
	}{
		TotalTests:         atomic.LoadInt64(&ept.metrics.totalTests),
		SuccessfulTests:    atomic.LoadInt64(&ept.metrics.successfulTests),
		FailedTests:        atomic.LoadInt64(&ept.metrics.failedTests),
		SuccessRate:        float64(atomic.LoadInt64(&ept.metrics.successfulTests)) / float64(max(1, atomic.LoadInt64(&ept.metrics.totalTests))) * 100,
		AvgResponseTime:    atomic.LoadFloat64(&ept.metrics.avgResponseTime),
		MemoryUsageMB:      atomic.LoadInt64(&ept.memoryUsage),
		ActiveProcesses:    atomic.LoadInt64(&ept.activeProcesses),
		UptimeSeconds:      int64(time.Since(ept.metrics.startTime).Seconds()),
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(metrics)
}

func max(a, b int64) int64 {
	if a > b {
		return a
	}
	return b
}

// Add new result type for resource exhaustion
const (
	ResultResourceExhausted TestResult = "resource_exhausted"
)