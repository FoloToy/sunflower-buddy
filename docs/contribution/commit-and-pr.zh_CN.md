<p align="right"><a href="commit-and-pr.md">English</a> · <strong>简体中文</strong></p>

# 提交与 PR 规范

## Commit

- 使用英文祈使句和 Conventional Commit 标题，例如
  `feat(bsp): add RTC alarm wrapper`。
- 一个 commit 只包含一项可独立 review 的修改，描述最终差异，不记录调试过程。
- 提交前检查完整 diff，运行适用验证，并排除凭据、本地配置、构建产物和无关文件。
- 用户可见行为或兼容性变化需要更新变更记录。
- 获得 commit 授权不等于获得 push、发布、合并、打 tag 或创建 Release 的授权。

## Pull Request

- 写明 Sunflower Buddy 硬件版本，以及引脚、供电、codec、ADC、UART、Flash 或时序影响。
- 分别报告 Build、Host tests、Device tests 和 Unverified。
- 硬件验证应附串口日志或测量结果；修改电机过流阈值、音频增益、模组波特率或低功耗
  行为时还应说明准确测试条件。
- Wi-Fi/BLUFI 修改应记录手机 App 版本、热点频段/安全模式、固件 commit、清空/已保存
  Station 配置、重复重置次数和断线原因，并脱敏凭据与标识。
- 编译或主机测试通过不能描述为真机验证通过。
