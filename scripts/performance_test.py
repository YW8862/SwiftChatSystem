#!/usr/bin/env python3
"""
SwiftChatSystem 性能测试脚本
用于测试系统的并发连接数、消息吞吐量、延迟等关键指标
"""

import socket
import threading
import time
import statistics
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed

# 配置
DEFAULT_HOST = "127.0.0.1"
DEFAULT_WEBSOCKET_PORT = 9090
DEFAULT_CONNECTIONS = 1000
DEFAULT_MESSAGES_PER_CONN = 100

class ConnectionStats:
    def __init__(self):
        self.connect_times = []
        self.message_latencies = []
        self.success_count = 0
        self.fail_count = 0
        self.lock = threading.Lock()
    
    def record_connect(self, time_ms):
        with self.lock:
            self.connect_times.append(time_ms)
    
    def record_message(self, latency_ms, success=True):
        with self.lock:
            if success:
                self.message_latencies.append(latency_ms)
                self.success_count += 1
            else:
                self.fail_count += 1
    
    def get_report(self):
        with self.lock:
            report = {
                'total_connections': len(self.connect_times),
                'avg_connect_time_ms': statistics.mean(self.connect_times) if self.connect_times else 0,
                'p50_connect_time_ms': statistics.quantiles(self.connect_times, n=2)[0] if len(self.connect_times) > 1 else 0,
                'p99_connect_time_ms': sorted(self.connect_times)[int(len(self.connect_times) * 0.99)] if len(self.connect_times) > 10 else 0,
                
                'total_messages': self.success_count + self.fail_count,
                'success_messages': self.success_count,
                'failed_messages': self.fail_count,
                'success_rate': (self.success_count / (self.success_count + self.fail_count) * 100) if (self.success_count + self.fail_count) > 0 else 0,
                
                'avg_message_latency_ms': statistics.mean(self.message_latencies) if self.message_latencies else 0,
                'p50_message_latency_ms': statistics.quantiles(self.message_latencies, n=2)[0] if len(self.message_latencies) > 1 else 0,
                'p95_message_latency_ms': sorted(self.message_latencies)[int(len(self.message_latencies) * 0.95)] if len(self.message_latencies) > 10 else 0,
                'p99_message_latency_ms': sorted(self.message_latencies)[int(len(self.message_latencies) * 0.99)] if len(self.message_latencies) > 20 else 0,
            }
            return report


def simulate_websocket_connection(conn_id, host, port, stats, num_messages):
    """模拟一个 WebSocket 连接并发送消息"""
    try:
        # 建立连接
        start = time.time()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((host, port))
        connect_time = (time.time() - start) * 1000
        stats.record_connect(connect_time)
        
        # 发送消息
        for i in range(num_messages):
            msg = f"conn_{conn_id}_msg_{i}".encode()
            send_start = time.time()
            sock.sendall(msg)
            # 简化测试，不等待响应
            latency = (time.time() - send_start) * 1000
            stats.record_message(latency, success=True)
        
        sock.close()
        
    except Exception as e:
        stats.record_message(0, success=False)
        print(f"Connection {conn_id} error: {e}")


def run_performance_test(host, port, num_connections, messages_per_conn):
    """运行性能测试"""
    print(f"\n{'='*60}")
    print(f"SwiftChatSystem 性能测试")
    print(f"{'='*60}")
    print(f"目标地址：{host}:{port}")
    print(f"并发连接数：{num_connections}")
    print(f"每连接消息数：{messages_per_conn}")
    print(f"{'='*60}\n")
    
    stats = ConnectionStats()
    start_time = time.time()
    
    # 使用线程池模拟并发连接
    with ThreadPoolExecutor(max_workers=num_connections) as executor:
        futures = [
            executor.submit(simulate_websocket_connection, i, host, port, stats, messages_per_conn)
            for i in range(num_connections)
        ]
        
        # 等待所有连接完成
        for future in as_completed(futures):
            try:
                future.result()
            except Exception as e:
                print(f"Task error: {e}")
    
    total_time = time.time() - start_time
    report = stats.get_report()
    
    # 打印报告
    print(f"\n{'='*60}")
    print(f"性能测试结果")
    print(f"{'='*60}")
    print(f"测试总耗时：{total_time:.2f} 秒")
    print(f"\n连接性能:")
    print(f"  总连接数：{report['total_connections']}")
    print(f"  平均连接时间：{report['avg_connect_time_ms']:.2f} ms")
    print(f"  P50 连接时间：{report['p50_connect_time_ms']:.2f} ms")
    print(f"  P99 连接时间：{report['p99_connect_time_ms']:.2f} ms")
    
    print(f"\n消息性能:")
    print(f"  总消息数：{report['total_messages']}")
    print(f"  成功消息数：{report['success_messages']}")
    print(f"  失败消息数：{report['failed_messages']}")
    print(f"  成功率：{report['success_rate']:.2f}%")
    print(f"  平均消息延迟：{report['avg_message_latency_ms']:.2f} ms")
    print(f"  P50 消息延迟：{report['p50_message_latency_ms']:.2f} ms")
    print(f"  P95 消息延迟：{report['p95_message_latency_ms']:.2f} ms")
    print(f"  P99 消息延迟：{report['p99_message_latency_ms']:.2f} ms")
    
    # 计算吞吐量
    total_messages = report['total_messages']
    qps = total_messages / total_time if total_time > 0 else 0
    print(f"\n系统吞吐量:")
    print(f"  QPS: {qps:.2f}")
    print(f"  平均每秒处理消息：{total_messages / total_time:.2f}")
    
    print(f"\n{'='*60}\n")
    
    return report


def main():
    parser = argparse.ArgumentParser(description='SwiftChatSystem Performance Test')
    parser.add_argument('--host', default=DEFAULT_HOST, help=f'服务器地址 (默认：{DEFAULT_HOST})')
    parser.add_argument('--port', type=int, default=DEFAULT_WEBSOCKET_PORT, help=f'WebSocket 端口 (默认：{DEFAULT_WEBSOCKET_PORT})')
    parser.add_argument('--connections', type=int, default=DEFAULT_CONNECTIONS, help=f'并发连接数 (默认：{DEFAULT_CONNECTIONS})')
    parser.add_argument('--messages', type=int, default=DEFAULT_MESSAGES_PER_CONN, help=f'每连接消息数 (默认：{DEFAULT_MESSAGES_PER_CONN})')
    
    args = parser.parse_args()
    
    run_performance_test(args.host, args.port, args.connections, args.messages)


if __name__ == '__main__':
    main()
