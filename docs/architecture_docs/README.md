# 软件架构与详细设计文档

## 概述

本目录包含自动驾驶数据闭环系统的软件架构与详细设计文档。

## 文件说明

- [software_architecture_and_detailed_design.md](file:///workspaces/ad_data_closed_loop/docs/architecture_docs/software_architecture_and_detailed_design.md) - Markdown格式的完整文档
- [software_architecture_and_detailed_design.pdf](file:///workspaces/ad_data_closed_loop/docs/architecture_docs/software_architecture_and_detailed_design.pdf) - PDF格式的完整文档

## 文档内容

该文档详细描述了自动驾驶数据闭环系统的软件架构和设计，包括：

1. 系统整体架构设计
2. 各模块详细设计
3. 状态机设计与实现
4. 数据采集与导航规划模块集成
5. 系统工作流程
6. 关键技术实现
7. 性能优化策略
8. 错误处理与恢复机制
9. 测试策略
10. 部署与运维说明

## 生成方法

如需重新生成PDF文档，可以使用以下命令：

```bash
cd /workspaces/ad_data_closed_loop/docs/architecture_docs
pandoc -f markdown -t pdf -o software_architecture_and_detailed_design.pdf software_architecture_and_detailed_design.md --toc --toc-depth=2 -V geometry:margin=1in -V fontsize=12pt --pdf-engine=xelatex -V CJKmainfont='Noto Sans CJK SC'
```