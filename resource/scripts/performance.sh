#!/bin/bash

# 配置参数
PROCESS_NAMES=("dcp_app")           # 监控的进程名列表
DURATION=60                             # 监控时长（秒）
REPORT_FILE="perf_report.txt"           # 报告文件
LOG_FILE="perf_monitor.log"             # 详细日志文件

# 获取系统资源信息
TOTAL_CORES=12 #${nproc}
TOTAL_MEM_KB=$(awk '/MemTotal/ {print $2}' /proc/meminfo)
TOTAL_MEM_GB=$(echo "scale=2; $TOTAL_MEM_KB / 1024 / 1024" | bc)

# 获取进程PID映射
declare -A PROCESS_MAP
for name in "${PROCESS_NAMES[@]}"; do
  pids=$(pgrep -f "$name" | tr '\n' ' ')
  [ -z "$pids" ] && echo "警告：未找到进程 $name" || PROCESS_MAP["$name"]=$pids
done

[ ${#PROCESS_MAP[@]} -eq 0 ] && { echo "错误：未找到任何目标进程！"; exit 1; }

# 初始化数据结构
declare -A CPU_RAW CPU_NORM MEM_RAW_KB MEM_NORM GPU_USAGE
end_time=$((SECONDS+DURATION))

# 初始化文件
> "$LOG_FILE"
> "$REPORT_FILE"

# 生成日志头
echo -n "时间戳" >> "$LOG_FILE"
for name in "${!PROCESS_MAP[@]}"; do
  printf " | %-20s CPU%-6s MEM%-6s GPU" "$name" "" "" >> "$LOG_FILE"
done
echo "" >> "$LOG_FILE"

# 主监控循环
while [ $SECONDS -lt $end_time ]; do
  timestamp=$(date +"%Y-%m-%d %H:%M:%S")
  log_entry="$timestamp"

  for name in "${!PROCESS_MAP[@]}"; do
    pids=${PROCESS_MAP[$name]}

    # 计算CPU和内存总和
    cpu_sum=0
    mem_sum_kb=0
    for pid in $pids; do
      cpu_sum=$(echo "$cpu_sum + $(ps -p $pid -o %cpu= 2>/dev/null)" | bc)
      mem_sum_kb=$(echo "$mem_sum_kb + $(ps -p $pid -o rss= 2>/dev/null)" | bc)
    done

    # 记录CPU数据
    CPU_RAW["$name"]+="$cpu_sum "
    CPU_NORM["$name"]+="$(echo "scale=2; $cpu_sum / $TOTAL_CORES" | bc) "

    # 记录内存数据
    MEM_RAW_KB["$name"]+="$mem_sum_kb "
    MEM_NORM["$name"]+="$(echo "scale=4; $mem_sum_kb / $TOTAL_MEM_KB * 100" | bc) "

    # 获取GPU数据
#    first_pid=$(echo $pids | awk '{print $1}')
#    gpu_val=$(nvidia-smi --query-compute-apps=pid,utilization.gpu --format=csv,noheader,nounits 2>/dev/null |
#              awk -F', ' -v pid="$first_pid" '$1 == pid {print $2}')
#    GPU_USAGE["$name"]+="${gpu_val:-0} "

    # 构建日志条目
    mem_gb=$(echo "scale=2; $mem_sum_kb / 1024 / 1024" | bc)
    log_entry+=$(printf " | %-20s %-6.1f %-6.2f %-4s" "$name" $cpu_sum $mem_gb "${gpu_val:-N/A}%")
  done

  echo "$log_entry" >> "$LOG_FILE"
  sleep 1
done

# 生成报告
{
  # 表头
  echo "完整资源监控报告"
  echo "系统资源 | CPU核心数: $TOTAL_CORES | 总内存: ${TOTAL_MEM_GB}GB"
  echo "监控时长: $DURATION 秒 | 生成时间: $(date +'%Y-%m-%d %H:%M:%S')"
  echo "============================================================================================================================================================================="
  printf "%-20s | %-12s | %-14s | %-14s | %-14s | %-14s | %-14s\n" \
         "进程名" "CPU峰值(%)" "CPU均值(%)" "CPU归一化峰值" "CPU均值(归一化)" "内存峰值(GB)" "内存均值(GB)"
  echo "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------"

  # 表格数据
  for name in "${!PROCESS_MAP[@]}"; do
    # CPU计算
    raw_cpu=(${CPU_RAW[$name]})
    norm_cpu=(${CPU_NORM[$name]})
    max_cpu_raw=$(printf "%s\n" "${raw_cpu[@]}" | sort -nr | head -1)
    avg_cpu_raw=$(echo "${raw_cpu[@]}" | tr ' ' '\n' | awk '{sum+=$1} END{printf "%.1f", sum/NR}')
    max_cpu_norm=$(printf "%s\n" "${norm_cpu[@]}" | sort -nr | head -1)
    avg_cpu_norm=$(echo "${norm_cpu[@]}" | tr ' ' '\n' | awk '{sum+=$1} END{printf "%.2f", sum/NR}')

    # 内存计算
    raw_mem_kb=(${MEM_RAW_KB[$name]})
    norm_mem=(${MEM_NORM[$name]})

    max_mem_kb=$(printf "%s\n" "${raw_mem_kb[@]}" | sort -nr | head -1)
    max_mem_gb=$(echo "scale=2; $max_mem_kb / 1024 / 1024" | bc)
    avg_mem_kb=$(echo "${raw_mem_kb[@]}" | tr ' ' '\n' | awk '{sum+=$1} END{print sum/NR}')
    avg_mem_gb=$(echo "scale=2; $avg_mem_kb / 1024 / 1024" | bc)

    max_mem_percent=$(printf "%s\n" "${norm_mem[@]}" | sort -nr | head -1)
    avg_mem_percent=$(echo "${norm_mem[@]}" | tr ' ' '\n' | awk '{sum+=$1} END{printf "%.2f", sum/NR}')

    # GPU计算
    gpu_values=(${GPU_USAGE[$name]})
    valid_gpu=($(printf "%s\n" "${gpu_values[@]}" | grep -vE '^0$'))
    max_gpu=$( [ ${#valid_gpu[@]} -gt 0 ] && printf "%s\n" "${valid_gpu[@]}" | sort -nr | head -1 || echo "N/A" )

    # 输出行
    printf "%-20s | %-12s | %-14s | %-14s | %-14s | %-14s | %-14s | %-14s | %-14s\n" \
      "$name" \
      "${max_cpu_raw}%" \
      "${avg_cpu_raw}%" \
      "${max_cpu_norm}%" \
      "${avg_cpu_norm}%" \
      "${max_mem_gb}GB" \
      "${avg_mem_gb}GB"
  done

  # 表尾注释
  echo "============================================================================================================================================================================="
  echo "注："
  echo "1. CPU归一化值 = 原始值 / 总核心数"
  echo "2. 内存百分比 = 进程内存使用量 / 系统总内存 * 100"
} > "$REPORT_FILE"

echo "监控完成！报告路径: $REPORT_FILE"
