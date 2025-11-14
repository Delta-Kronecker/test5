#!/usr/bin/env python3
"""
Advanced CPU Information Collector for GitHub Actions
Uses multiple methods to gather comprehensive CPU details
"""

import platform
import psutil
import socket
import datetime
import os
import json
import subprocess
import re
import multiprocessing

class CPUInfoCollector:
    def __init__(self):
        self.cpu_info = {}
        self.timestamp = datetime.datetime.now().isoformat()
    
    def collect_all_cpu_info(self):
        """Collect CPU information using all available methods"""
        print("🔍 Collecting comprehensive CPU information using multiple methods...")
        
        self.cpu_info = {
            'timestamp': self.timestamp,
            'basic_info': self.get_basic_cpu_info(),
            'detailed_info': self.get_detailed_cpu_info(),
            'frequency_info': self.get_cpu_frequency_all_methods(),
            'cache_info': self.get_cpu_cache_info(),
            'architecture_info': self.get_cpu_architecture_info(),
            'flags_info': self.get_cpu_flags(),
            'topology_info': self.get_cpu_topology(),
            'performance_info': self.get_cpu_performance_metrics()
        }
        
        return self.cpu_info
    
    def get_basic_cpu_info(self):
        """Get basic CPU information using platform module"""
        info = {
            'processor_name': platform.processor(),
            'architecture': platform.architecture()[0],
            'machine': platform.machine(),
            'platform': platform.platform(),
            'python_compiler': platform.python_compiler()
        }
        
        # Try to get more detailed processor info
        try:
            if hasattr(platform, 'uname'):
                uname = platform.uname()
                info['system'] = uname.system
                info['node'] = uname.node
                info['release'] = uname.release
                info['version'] = uname.version
                info['machine'] = uname.machine
                info['processor'] = uname.processor
        except Exception as e:
            print(f"⚠️ Could not get uname info: {e}")
            
        return info
    
    def get_detailed_cpu_info(self):
        """Get detailed CPU information using psutil"""
        info = {
            'physical_cores': psutil.cpu_count(logical=False),
            'logical_cores': psutil.cpu_count(logical=True),
            'cpu_usage_percent': psutil.cpu_percent(interval=1),
            'cpu_usage_per_core': psutil.cpu_percent(interval=1, percpu=True)
        }
        
        # CPU times
        try:
            times = psutil.cpu_times()
            info.update({
                'user_time': times.user,
                'system_time': times.system,
                'idle_time': times.idle,
                'nice_time': getattr(times, 'nice', 0),
                'iowait_time': getattr(times, 'iowait', 0),
                'irq_time': getattr(times, 'irq', 0),
                'softirq_time': getattr(times, 'softirq', 0),
                'steal_time': getattr(times, 'steal', 0),
                'guest_time': getattr(times, 'guest', 0)
            })
        except Exception as e:
            print(f"⚠️ Could not get CPU times: {e}")
        
        # CPU stats
        try:
            stats = psutil.cpu_stats()
            info.update({
                'ctx_switches': stats.ctx_switches,
                'interrupts': stats.interrupts,
                'soft_interrupts': stats.soft_interrupts,
                'syscalls': getattr(stats, 'syscalls', 0)
            })
        except Exception as e:
            print(f"⚠️ Could not get CPU stats: {e}")
            
        return info
    
    def get_cpu_frequency_all_methods(self):
        """Get CPU frequency using all available methods"""
        frequency_info = {
            'methods_used': [],
            'results': {}
        }
        
        # Method 1: psutil
        try:
            freq = psutil.cpu_freq()
            if freq:
                frequency_info['methods_used'].append('psutil')
                frequency_info['results']['psutil'] = {
                    'current_mhz': freq.current,
                    'min_mhz': freq.min if freq.min and freq.min > 0 else 'Unknown',
                    'max_mhz': freq.max if freq.max and freq.max > 0 else 'Unknown'
                }
                print(f"✅ psutil method: Current={freq.current} MHz, Max={freq.max} MHz")
        except Exception as e:
            print(f"❌ psutil method failed: {e}")
        
        # Method 2: /proc/cpuinfo (Linux)
        try:
            proc_info = self._get_cpuinfo_frequency()
            if proc_info:
                frequency_info['methods_used'].append('/proc/cpuinfo')
                frequency_info['results']['proc_cpuinfo'] = proc_info
                print(f"✅ /proc/cpuinfo method: {proc_info}")
        except Exception as e:
            print(f"❌ /proc/cpuinfo method failed: {e}")
        
        # Method 3: lscpu command
        try:
            lscpu_info = self._get_lscpu_frequency()
            if lscpu_info:
                frequency_info['methods_used'].append('lscpu')
                frequency_info['results']['lscpu'] = lscpu_info
                print(f"✅ lscpu method: {lscpu_info}")
        except Exception as e:
            print(f"❌ lscpu method failed: {e}")
        
        # Method 4: cpufreq-info command
        try:
            cpufreq_info = self._get_cpufreq_info()
            if cpufreq_info:
                frequency_info['methods_used'].append('cpufreq')
                frequency_info['results']['cpufreq'] = cpufreq_info
                print(f"✅ cpufreq method: {cpufreq_info}")
        except Exception as e:
            print(f"❌ cpufreq method failed: {e}")
        
        # Method 5: dmidecode (requires root)
        try:
            dmidecode_info = self._get_dmidecode_cpu_info()
            if dmidecode_info:
                frequency_info['methods_used'].append('dmidecode')
                frequency_info['results']['dmidecode'] = dmidecode_info
                print(f"✅ dmidecode method: {dmidecode_info}")
        except Exception as e:
            print(f"❌ dmidecode method failed: {e}")
        
        # Method 6: Using multiprocessing
        try:
            mp_info = self._get_mp_cpu_info()
            if mp_info:
                frequency_info['methods_used'].append('multiprocessing')
                frequency_info['results']['multiprocessing'] = mp_info
                print(f"✅ multiprocessing method: {mp_info}")
        except Exception as e:
            print(f"❌ multiprocessing method failed: {e}")
        
        # Determine best available frequency values
        frequency_info['best_estimate'] = self._get_best_frequency_estimate(frequency_info['results'])
        
        return frequency_info
    
    def _get_cpuinfo_frequency(self):
        """Get CPU frequency from /proc/cpuinfo"""
        try:
            with open('/proc/cpuinfo', 'r') as f:
                content = f.read()
            
            cpu_info = {}
            
            # Look for frequency information
            freq_match = re.search(r'cpu MHz\s*:\s*([\d.]+)', content)
            if freq_match:
                cpu_info['current_mhz'] = float(freq_match.group(1))
            
            # Look for model name which might contain frequency
            model_match = re.search(r'model name\s*:\s*(.+)', content)
            if model_match:
                model_name = model_match.group(1)
                cpu_info['model_name'] = model_name
                
                # Try to extract frequency from model name
                ghz_match = re.search(r'([\d.]+)\s*GHz', model_name, re.IGNORECASE)
                if ghz_match:
                    cpu_info['advertised_ghz'] = float(ghz_match.group(1))
                    cpu_info['advertised_mhz'] = float(ghz_match.group(1)) * 1000
            
            return cpu_info if cpu_info else None
            
        except Exception as e:
            print(f"⚠️ Error reading /proc/cpuinfo: {e}")
            return None
    
    def _get_lscpu_frequency(self):
        """Get CPU frequency using lscpu command"""
        try:
            result = subprocess.run(['lscpu'], capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                lscpu_output = result.stdout
                cpu_info = {}
                
                # Parse CPU max MHz
                max_match = re.search(r'CPU max MHz:\s*([\d.]+)', lscpu_output)
                if max_match:
                    cpu_info['max_mhz'] = float(max_match.group(1))
                
                # Parse CPU min MHz
                min_match = re.search(r'CPU min MHz:\s*([\d.]+)', lscpu_output)
                if min_match:
                    cpu_info['min_mhz'] = float(min_match.group(1))
                
                # Parse CPU current MHz
                current_match = re.search(r'CPU MHz:\s*([\d.]+)', lscpu_output)
                if current_match:
                    cpu_info['current_mhz'] = float(current_match.group(1))
                
                # Parse model name
                model_match = re.search(r'Model name:\s*(.+)', lscpu_output)
                if model_match:
                    cpu_info['model_name'] = model_match.group(1).strip()
                
                return cpu_info if cpu_info else None
        except Exception as e:
            print(f"⚠️ lscpu command failed: {e}")
            return None
    
    def _get_cpufreq_info(self):
        """Get CPU frequency using cpufreq-info"""
        try:
            result = subprocess.run(['cpufreq-info'], capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                output = result.stdout
                cpu_info = {}
                
                # Parse current frequency
                current_match = re.search(r'current CPU frequency is ([\d.]+) MHz', output)
                if current_match:
                    cpu_info['current_mhz'] = float(current_match.group(1))
                
                # Parse policy info
                policy_match = re.search(r'policy:\s*(\d+) MHz - ([\d.]+) MHz', output)
                if policy_match:
                    cpu_info['min_mhz'] = float(policy_match.group(1))
                    cpu_info['max_mhz'] = float(policy_match.group(2))
                
                return cpu_info if cpu_info else None
        except Exception as e:
            print(f"⚠️ cpufreq-info command failed: {e}")
            return None
    
    def _get_dmidecode_cpu_info(self):
        """Get CPU information using dmidecode (requires root)"""
        try:
            result = subprocess.run(['sudo', 'dmidecode', '-t', 'processor'], 
                                  capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                output = result.stdout
                cpu_info = {}
                
                # Parse max speed
                max_match = re.search(r'Max Speed:\s*([\d.]+) MHz', output)
                if max_match:
                    cpu_info['max_mhz'] = float(max_match.group(1))
                
                # Parse current speed
                current_match = re.search(r'Current Speed:\s*([\d.]+) MHz', output)
                if current_match:
                    cpu_info['current_mhz'] = float(current_match.group(1))
                
                return cpu_info if cpu_info else None
        except Exception as e:
            print(f"⚠️ dmidecode command failed: {e}")
            return None
    
    def _get_mp_cpu_info(self):
        """Get CPU information using multiprocessing"""
        try:
            cpu_info = {
                'cpu_count': multiprocessing.cpu_count()
            }
            return cpu_info
        except Exception as e:
            print(f"⚠️ multiprocessing method failed: {e}")
            return None
    
    def _get_best_frequency_estimate(self, results):
        """Determine the best frequency estimate from all methods"""
        best = {'current_mhz': None, 'max_mhz': None, 'min_mhz': None}
        
        # Priority order of methods
        methods_priority = ['lscpu', 'cpufreq', 'psutil', 'proc_cpuinfo', 'dmidecode']
        
        for method in methods_priority:
            if method in results:
                data = results[method]
                if best['current_mhz'] is None and 'current_mhz' in data and data['current_mhz'] and data['current_mhz'] != 'Unknown':
                    if isinstance(data['current_mhz'], (int, float)):
                        best['current_mhz'] = data['current_mhz']
                if best['max_mhz'] is None and 'max_mhz' in data and data['max_mhz'] and data['max_mhz'] != 'Unknown':
                    if isinstance(data['max_mhz'], (int, float)):
                        best['max_mhz'] = data['max_mhz']
                if best['min_mhz'] is None and 'min_mhz' in data and data['min_mhz'] and data['min_mhz'] != 'Unknown':
                    if isinstance(data['min_mhz'], (int, float)):
                        best['min_mhz'] = data['min_mhz']
        
        return best
    
    def get_cpu_cache_info(self):
        """Get CPU cache information"""
        cache_info = {}
        
        try:
            # Try to get cache info from /sys/devices/system/cpu/
            cpu0_path = '/sys/devices/system/cpu/cpu0/cache/'
            if os.path.exists(cpu0_path):
                cache_dirs = [d for d in os.listdir(cpu0_path) if d.startswith('index')]
                cache_info['cache_levels'] = len(cache_dirs)
                
                for cache_dir in cache_dirs:
                    cache_path = os.path.join(cpu0_path, cache_dir)
                    try:
                        with open(os.path.join(cache_path, 'level'), 'r') as f:
                            level = f.read().strip()
                        with open(os.path.join(cache_path, 'type'), 'r') as f:
                            cache_type = f.read().strip()
                        with open(os.path.join(cache_path, 'size'), 'r') as f:
                            size = f.read().strip()
                        
                        cache_info[f'level_{level}_{cache_type}'] = size
                    except Exception as e:
                        print(f"⚠️ Could not read cache info for {cache_dir}: {e}")
        except Exception as e:
            print(f"⚠️ Could not get cache info: {e}")
        
        return cache_info
    
    def get_cpu_flags(self):
        """Get CPU flags and features"""
        flags_info = {}
        
        try:
            with open('/proc/cpuinfo', 'r') as f:
                content = f.read()
            
            flags_match = re.search(r'flags\s*:\s*(.+)', content)
            if flags_match:
                flags = flags_match.group(1).split()
                flags_info['flags_count'] = len(flags)
                flags_info['flags'] = flags
                
                # Check for important features
                important_flags = ['avx', 'avx2', 'sse', 'sse2', 'sse3', 'sse4', 
                                 'aes', 'vmx', 'svm', 'hypervisor', 'tsc']
                flags_info['important_features'] = [flag for flag in important_flags if flag in flags]
        except Exception as e:
            print(f"⚠️ Could not get CPU flags: {e}")
        
        return flags_info
    
    def get_cpu_topology(self):
        """Get CPU topology information"""
        topology = {}
        
        try:
            result = subprocess.run(['lscpu', '-p'], capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                topology['lscpu_topology'] = result.stdout.split('\n')[:10]  # First 10 lines
        except Exception as e:
            print(f"⚠️ Could not get CPU topology: {e}")
        
        # Get NUMA info if available
        try:
            result = subprocess.run(['numactl', '--hardware'], capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                topology['numa_info'] = result.stdout
        except Exception as e:
            print(f"⚠️ Could not get NUMA info: {e}")
        
        return topology
    
    def get_cpu_architecture_info(self):
        """Get CPU architecture specific information"""
        arch_info = {}
        
        try:
            result = subprocess.run(['lscpu'], capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                lines = result.stdout.split('\n')
                for line in lines:
                    if ':' in line:
                        key, value = line.split(':', 1)
                        arch_info[key.strip()] = value.strip()
        except Exception as e:
            print(f"⚠️ Could not get architecture info: {e}")
        
        return arch_info
    
    def get_cpu_performance_metrics(self):
        """Get CPU performance metrics"""
        performance = {}
        
        # Measure CPU performance with a simple calculation
        import time
        start_time = time.time()
        
        # Simple CPU benchmark - calculate primes
        def is_prime(n):
            if n < 2:
                return False
            for i in range(2, int(n**0.5) + 1):
                if n % i == 0:
                    return False
            return True
        
        # Count primes in a range (simple benchmark)
        prime_count = 0
        for i in range(2, 10000):
            if is_prime(i):
                prime_count += 1
        
        end_time = time.time()
        
        performance['prime_calculation_time'] = end_time - start_time
        performance['primes_found'] = prime_count
        
        # Get load average
        try:
            load_avg = os.getloadavg()
            performance['load_1min'] = load_avg[0]
            performance['load_5min'] = load_avg[1]
            performance['load_15min'] = load_avg[2]
        except Exception as e:
            print(f"⚠️ Could not get load average: {e}")
        
        return performance
    
    def _format_frequency(self, mhz_value):
        """Format frequency value safely"""
        if mhz_value is None or mhz_value == 'Unknown':
            return "Unknown"
        try:
            if isinstance(mhz_value, (int, float)):
                return f"{mhz_value} MHz ({mhz_value/1000:.2f} GHz)"
            else:
                return str(mhz_value)
        except:
            return str(mhz_value)
    
    def generate_comprehensive_report(self):
        """Generate comprehensive CPU report"""
        report = []
        report.append("=" * 80)
        report.append("🖥️ COMPREHENSIVE CPU INFORMATION REPORT")
        report.append(f"📅 Generated at: {self.timestamp}")
        report.append("=" * 80)
        
        # Basic Info
        basic = self.cpu_info['basic_info']
        report.append("\n🎯 BASIC CPU INFORMATION:")
        report.append("-" * 50)
        for key, value in basic.items():
            report.append(f"  {key.replace('_', ' ').title()}: {value}")
        
        # Detailed Info
        detailed = self.cpu_info['detailed_info']
        report.append("\n⚡ DETAILED CPU INFORMATION:")
        report.append("-" * 50)
        report.append(f"  Physical Cores: {detailed['physical_cores']}")
        report.append(f"  Logical Cores: {detailed['logical_cores']}")
        report.append(f"  CPU Usage: {detailed['cpu_usage_percent']}%")
        
        # Frequency Information
        freq = self.cpu_info['frequency_info']
        report.append("\n📊 CPU FREQUENCY INFORMATION:")
        report.append("-" * 50)
        report.append(f"  Methods Used: {', '.join(freq['methods_used'])}")
        
        best = freq['best_estimate']
        report.append(f"  🎯 Best Current Frequency: {self._format_frequency(best['current_mhz'])}")
        report.append(f"  🎯 Best Max Frequency: {self._format_frequency(best['max_mhz'])}")
        report.append(f"  🎯 Best Min Frequency: {self._format_frequency(best['min_mhz'])}")
        
        # Show all frequency results
        for method, data in freq['results'].items():
            report.append(f"\n  📋 {method.upper()} Results:")
            for key, value in data.items():
                if 'mhz' in key:
                    report.append(f"    {key}: {self._format_frequency(value)}")
                elif key == 'model_name':
                    report.append(f"    {key}: {value}")
        
        # Cache Information
        cache = self.cpu_info['cache_info']
        if cache:
            report.append("\n💾 CPU CACHE INFORMATION:")
            report.append("-" * 50)
            for key, value in cache.items():
                report.append(f"  {key.replace('_', ' ').title()}: {value}")
        
        # Flags Information
        flags = self.cpu_info['flags_info']
        if flags.get('flags'):
            report.append("\n🚩 CPU FLAGS & FEATURES:")
            report.append("-" * 50)
            report.append(f"  Total Flags: {flags.get('flags_count', 0)}")
            if flags.get('important_features'):
                report.append(f"  Important Features: {', '.join(flags['important_features'])}")
        
        # Performance Metrics
        perf = self.cpu_info['performance_info']
        report.append("\n📈 CPU PERFORMANCE METRICS:")
        report.append("-" * 50)
        report.append(f"  Prime Calculation Time: {perf['prime_calculation_time']:.4f} seconds")
        report.append(f"  Primes Found: {perf['primes_found']}")
        if 'load_1min' in perf:
            report.append(f"  Load Average (1min): {perf['load_1min']:.2f}")
        
        report.append("\n" + "=" * 80)
        report.append("✅ Comprehensive CPU report completed successfully!")
        
        return "\n".join(report)
    
    def save_reports(self):
        """Save reports to files"""
        # Text report
        with open('cpu_detailed_report.txt', 'w') as f:
            f.write(self.generate_comprehensive_report())
        
        # JSON report
        with open('cpu_detailed_report.json', 'w') as f:
            json.dump(self.cpu_info, f, indent=2, default=str)
        
        print("\n📁 Reports saved:")
        print("  - cpu_detailed_report.txt")
        print("  - cpu_detailed_report.json")

def main():
    """Main execution function"""
    try:
        collector = CPUInfoCollector()
        collector.collect_all_cpu_info()
        
        # Print to console
        print(collector.generate_comprehensive_report())
        
        # Save files
        collector.save_reports()
        
    except Exception as e:
        print(f"❌ Error collecting CPU information: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())
