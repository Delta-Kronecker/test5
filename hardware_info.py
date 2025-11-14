#!/usr/bin/env python3
"""
Script to collect and report hardware information
Compatible with GitHub Actions runners
"""

import platform
import psutil
import socket
import datetime
import os
import sys

def get_system_info():
    """Get basic system information"""
    info = {}
    
    # System information
    info['Platform'] = platform.platform()
    info['System'] = platform.system()
    info['Release'] = platform.release()
    info['Version'] = platform.version()
    info['Architecture'] = platform.architecture()[0]
    info['Processor'] = platform.processor()
    
    # Machine information
    info['Machine'] = platform.machine()
    info['Hostname'] = socket.gethostname()
    
    return info

def get_cpu_info():
    """Get CPU information"""
    cpu_info = {}
    
    # CPU cores
    cpu_info['Physical Cores'] = psutil.cpu_count(logical=False)
    cpu_info['Total Cores'] = psutil.cpu_count(logical=True)
    
    # CPU frequencies
    try:
        freq = psutil.cpu_freq()
        cpu_info['Max Frequency'] = f"{freq.max:.2f} MHz" if freq else "N/A"
        cpu_info['Current Frequency'] = f"{freq.current:.2f} MHz" if freq else "N/A"
    except:
        cpu_info['Frequency'] = "N/A"
    
    # CPU usage
    cpu_info['CPU Usage'] = f"{psutil.cpu_percent(interval=1)}%"
    
    # CPU stats
    cpu_stats = psutil.cpu_stats()
    cpu_info['CPU Context Switches'] = cpu_stats.ctx_switches
    cpu_info['CPU Interrupts'] = cpu_stats.interrupts
    
    return cpu_info

def get_memory_info():
    """Get memory information"""
    memory_info = {}
    
    # Virtual memory
    virt_mem = psutil.virtual_memory()
    memory_info['Total RAM'] = f"{virt_mem.total / (1024**3):.2f} GB"
    memory_info['Available RAM'] = f"{virt_mem.available / (1024**3):.2f} GB"
    memory_info['Used RAM'] = f"{virt_mem.used / (1024**3):.2f} GB"
    memory_info['RAM Usage'] = f"{virt_mem.percent}%"
    
    # Swap memory
    swap_mem = psutil.swap_memory()
    memory_info['Total Swap'] = f"{swap_mem.total / (1024**3):.2f} GB"
    memory_info['Used Swap'] = f"{swap_mem.used / (1024**3):.2f} GB"
    memory_info['Swap Usage'] = f"{swap_mem.percent}%"
    
    return memory_info

def get_disk_info():
    """Get disk information"""
    disk_info = {}
    
    # Disk partitions
    partitions = psutil.disk_partitions()
    disk_info['Partitions'] = len(partitions)
    
    disk_usage = []
    for partition in partitions:
        try:
            usage = psutil.disk_usage(partition.mountpoint)
            disk_usage.append({
                'Device': partition.device,
                'Mountpoint': partition.mountpoint,
                'File System': partition.fstype,
                'Total Size': f"{usage.total / (1024**3):.2f} GB",
                'Used': f"{usage.used / (1024**3):.2f} GB",
                'Free': f"{usage.free / (1024**3):.2f} GB",
                'Usage': f"{usage.percent}%"
            })
        except PermissionError:
            continue
    
    disk_info['Details'] = disk_usage
    return disk_info

def get_network_info():
    """Get network information"""
    network_info = {}
    
    # Network interfaces
    net_io = psutil.net_io_counters()
    network_info['Bytes Sent'] = f"{net_io.bytes_sent / (1024**2):.2f} MB"
    network_info['Bytes Received'] = f"{net_io.bytes_recv / (1024**2):.2f} MB"
    
    # Network interfaces details
    interfaces = psutil.net_if_addrs()
    network_info['Network Interfaces'] = list(interfaces.keys())
    
    return network_info

def get_github_actions_info():
    """Get GitHub Actions specific information"""
    actions_info = {}
    
    # Environment variables specific to GitHub Actions
    env_vars = [
        'GITHUB_ACTIONS',
        'GITHUB_RUN_ID',
        'GITHUB_RUN_NUMBER',
        'GITHUB_WORKFLOW',
        'GITHUB_ACTION',
        'GITHUB_ACTOR',
        'GITHUB_REPOSITORY',
        'GITHUB_EVENT_NAME',
        'RUNNER_OS',
        'RUNNER_NAME'
    ]
    
    for var in env_vars:
        actions_info[var] = os.getenv(var, 'Not set')
    
    return actions_info

def generate_report():
    """Generate comprehensive hardware report"""
    report = []
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    report.append("=" * 60)
    report.append(f"HARDWARE INFORMATION REPORT")
    report.append(f"Generated at: {timestamp}")
    report.append("=" * 60)
    
    # System Information
    report.append("\nSYSTEM INFORMATION:")
    report.append("-" * 40)
    system_info = get_system_info()
    for key, value in system_info.items():
        report.append(f"{key}: {value}")
    
    # CPU Information
    report.append("\nCPU INFORMATION:")
    report.append("-" * 40)
    cpu_info = get_cpu_info()
    for key, value in cpu_info.items():
        report.append(f"{key}: {value}")
    
    # Memory Information
    report.append("\nMEMORY INFORMATION:")
    report.append("-" * 40)
    memory_info = get_memory_info()
    for key, value in memory_info.items():
        report.append(f"{key}: {value}")
    
    # Disk Information
    report.append("\nDISK INFORMATION:")
    report.append("-" * 40)
    disk_info = get_disk_info()
    report.append(f"Number of Partitions: {disk_info['Partitions']}")
    for disk in disk_info['Details']:
        report.append(f"\nDevice: {disk['Device']}")
        report.append(f"  Mountpoint: {disk['Mountpoint']}")
        report.append(f"  File System: {disk['File System']}")
        report.append(f"  Total Size: {disk['Total Size']}")
        report.append(f"  Used: {disk['Used']}")
        report.append(f"  Free: {disk['Free']}")
        report.append(f"  Usage: {disk['Usage']}")
    
    # Network Information
    report.append("\nNETWORK INFORMATION:")
    report.append("-" * 40)
    network_info = get_network_info()
    for key, value in network_info.items():
        if key == 'Network Interfaces':
            report.append(f"{key}: {', '.join(value)}")
        else:
            report.append(f"{key}: {value}")
    
    # GitHub Actions Information
    report.append("\nGITHUB ACTIONS INFORMATION:")
    report.append("-" * 40)
    actions_info = get_github_actions_info()
    for key, value in actions_info.items():
        report.append(f"{key}: {value}")
    
    return "\n".join(report)

def main():
    """Main function"""
    try:
        print("Collecting hardware information...")
        
        # Generate the report
        report = generate_report()
        
        # Print to console
        print(report)
        
        # Save to file
        with open('hardware_report.txt', 'w') as f:
            f.write(report)
        
        print(f"\nReport saved to: hardware_report.txt")
        print("Hardware information collection completed successfully!")
        
    except Exception as e:
        print(f"Error collecting hardware information: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
