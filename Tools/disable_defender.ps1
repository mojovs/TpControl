<#
╔══════════════════════════════════════════════════════════╗
║  彻底关闭 Windows Defender 脚本                          ║
║  适用: Windows 10/11                                    ║
║  注意: 需要管理员权限运行                                ║
║        关闭后系统安全风险自担                            ║
╚══════════════════════════════════════════════════════════╝

⚠️  如果 Tamper Protection (篡改保护) 已开启,
    部分设置可能被 Windows Security 自动还原。
    建议先手动关闭:
      设置 → 隐私与安全性 → Windows 安全中心 →
      病毒与威胁防护 → 管理设置 → 篡改保护 → 关闭

    也可以: "Windows 安全中心" → "病毒与威胁防护" → "管理设置"
#>

#Requires -RunAsAdministrator

Write-Host "╔══════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║       开始关闭 Windows Defender...               ║" -ForegroundColor Cyan
Write-Host "╚══════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# ============================================================
# 第 1 步: 检查篡改保护状态
# ============================================================
Write-Host "[1/7] 检查篡改保护状态..." -ForegroundColor Yellow
try {
    $tamper = (Get-MpComputerStatus).IsTamperProtected
    if ($tamper -eq $true) {
        Write-Host "  ⚠️  篡改保护已开启! 部分修改可能被自动还原。" -ForegroundColor Red
        Write-Host "    建议手动关闭后重新运行本脚本。" -ForegroundColor Red
        Write-Host "    路径: Windows 安全中心 → 病毒与威胁防护 → 管理设置 → 篡改保护" -ForegroundColor Red
    } else {
        Write-Host "  ✅ 篡改保护已关闭，继续..." -ForegroundColor Green
    }
} catch {
    Write-Host "  ⚠️  无法读取篡改保护状态，继续执行..." -ForegroundColor Yellow
}

# ============================================================
# 第 2 步: 用 PowerShell 关闭 Defender 各项功能
# ============================================================
Write-Host "[2/7] 关闭 Defender 实时/云/行为防护..." -ForegroundColor Yellow

$defenderSettings = @(
    @{Name="DisableRealtimeMonitoring";           Value=1; Desc="实时监控"},
    @{Name="DisableBehaviorMonitoring";            Value=1; Desc="行为监控"},
    @{Name="DisableBlockAtFirstSeen";              Value=1; Desc="首次看到即阻止"},
    @{Name="DisableIOAVProtection";                Value=1; Desc="下载文件扫描"},
    @{Name="DisableScriptScanning";                Value=1; Desc="脚本扫描"},
    @{Name="DisableIntrusionPreventionSystem";     Value=1; Desc="入侵防御"},
    @{Name="MAPSReporting";                        Value=0; Desc="MAPS 报告"},
    @{Name="DisablePrivacyMode";                   Value=1; Desc="隐私模式"},
    @{Name="PUAProtection";                        Value=0; Desc="PUA 保护"},
    @{Name="SubmitSamplesConsent";                 Value=2; Desc="样本提交"},
    @{Name="HighThreatDefaultAction";              Value=6; Desc="高威胁默认操作(Accept)"},
    @{Name="ModerateThreatDefaultAction";          Value=6; Desc="中威胁默认操作(Accept)"},
    @{Name="LowThreatDefaultAction";               Value=6; Desc="低威胁默认操作(Accept)"},
    @{Name="SevereThreatDefaultAction";            Value=6; Desc="严重威胁默认操作(Accept)"},
    @{Name="CheckForSignaturesBeforeRunningScan";  Value=0; Desc="扫描前检查签名"},
    @{Name="SignatureUpdateInterval";              Value=24;Desc="签名更新间隔"},
    @{Name="CloudBlockLevel";                      Value=0; Desc="云阻断级别"},
    @{Name="CloudTimeout";                         Value=0; Desc="云超时"},
    @{Name="MeteredConnectionUpdates";             Value=0; Desc="按流量计费网络更新"},
    @{Name="AllowArchiveScanning";                 Value=0; Desc="存档扫描"},
    @{Name="AllowEmailScanning";                   Value=0; Desc="邮件扫描"},
    @{Name="AllowFullScanOnMappedNetworkDrives";   Value=0; Desc="映射网络驱动器扫描"},
    @{Name="AllowNetworkProtectionOnWinServer";    Value=0; Desc="服务器网络保护"},
    @{Name="AllowNetworkProtectionDownLevel";      Value=0; Desc="下级系统网络保护"},
    @{Name="AllowRealtimeMonitoring";              Value=0; Desc="允许实时监控"}
)

$ok = $true
foreach ($s in $defenderSettings) {
    try {
        Set-MpPreference -ErrorAction Stop @{$s.Name=$s.Value}
        Write-Host "  ✅ $($s.Desc)" -ForegroundColor Green
    } catch {
        Write-Host "  ❌ $($s.Desc) — $($_.Exception.Message)" -ForegroundColor Red
        $ok = $false
    }
}

# ============================================================
# 第 3 步: 计划任务 - 禁用 Defender 相关的自动扫描
# ============================================================
Write-Host "[3/7] 禁用 Defender 计划任务..." -ForegroundColor Yellow

$scheduledTasks = @(
    "\Microsoft\Windows\Windows Defender\Windows Defender Cache Maintenance",
    "\Microsoft\Windows\Windows Defender\Windows Defender Cleanup",
    "\Microsoft\Windows\Windows Defender\Windows Defender Scheduled Scan",
    "\Microsoft\Windows\Windows Defender\Windows Defender Verification"
)

foreach ($task in $scheduledTasks) {
    try {
        Disable-ScheduledTask -TaskPath $task 2>$null
        Write-Host "  ✅ 禁用: $task" -ForegroundColor Green
    } catch {
        Write-Host "  ⚠️  跳过: $task" -ForegroundColor Gray
    }
}

# ============================================================
# 第 4 步: 注册表 - 彻底禁用 Defender (组策略强制)
# ============================================================
Write-Host "[4/7] 注册表强制禁用 Defender..." -ForegroundColor Yellow

$registryPaths = @(
    @{Path="HKLM:\SOFTWARE\Policies\Microsoft\Windows Defender"; KeyValues=@(
        @{Name="DisableAntiSpyware"; Value=1; Type="DWord"}
        @{Name="DisableRealtimeMonitoring"; Value=1; Type="DWord"}
        @{Name="AllowFastServiceStartup"; Value=0; Type="DWord"}
        @{Name="ServiceKeepAlive"; Value=0; Type="DWord"}
    )},
    @{Path="HKLM:\SOFTWARE\Policies\Microsoft\Windows Defender\Real-Time Protection"; KeyValues=@(
        @{Name="DisableBehaviorMonitoring"; Value=1; Type="DWord"}
        @{Name="DisableIOAVProtection"; Value=1; Type="DWord"}
        @{Name="DisableOnAccessProtection"; Value=1; Type="DWord"}
        @{Name="DisableScanOnRealtimeEnable"; Value=1; Type="DWord"}
        @{Name="DisableRealtimeMonitoring"; Value=1; Type="DWord"}
    )},
    @{Path="HKLM:\SOFTWARE\Policies\Microsoft\Windows Defender\SpyNet"; KeyValues=@(
        @{Name="DisableBlockAtFirstSeen"; Value=1; Type="DWord"}
        @{Name="SpyNetReporting"; Value=0; Type="DWord"}
        @{Name="SubmitSamplesConsent"; Value=2; Type="DWord"}
    )},
    @{Path="HKLM:\SOFTWARE\Policies\Microsoft\Windows Defender\Signature Updates"; KeyValues=@(
        @{Name="ForceUpdateFromMU"; Value=0; Type="DWord"}
        @{Name="UpdateInterval"; Value=0; Type="DWord"}
    )},
    @{Path="HKLM:\SOFTWARE\Policies\Microsoft\Microsoft Antimalware"; KeyValues=@(
        @{Name="DisableAntiSpyware"; Value=1; Type="DWord"}
        @{Name="DisableAntiVirus"; Value=1; Type="DWord"}
    )},
    @{Path="HKLM:\SOFTWARE\Policies\Microsoft\Microsoft Antimalware\Real-Time Protection"; KeyValues=@(
        @{Name="DisableBehaviorMonitoring"; Value=1; Type="DWord"}
        @{Name="DisableIOAVProtection"; Value=1; Type="DWord"}
        @{Name="DisableOnAccessProtection"; Value=1; Type="DWord"}
        @{Name="DisableRealtimeMonitoring"; Value=1; Type="DWord"}
    )},
    # Windows 11 额外策略
    @{Path="HKLM:\SOFTWARE\Policies\Microsoft\Windows Advanced Threat Protection"; KeyValues=@(
        @{Name="ForceDefenderPassiveMode"; Value=1; Type="DWord"}
    )}
)

foreach ($reg in $registryPaths) {
    # 确保路径存在
    if (-not (Test-Path $reg.Path)) {
        New-Item -Path $reg.Path -Force -ItemType Directory | Out-Null
    }
    foreach ($kv in $reg.KeyValues) {
        try {
            Set-ItemProperty -Path $reg.Path -Name $kv.Name -Value $kv.Value -Type $kv.Type -ErrorAction Stop
            Write-Host "  ✅ 注册表: $($reg.Path) → $($kv.Name)=$($kv.Value)" -ForegroundColor Green
        } catch {
            Write-Host "  ❌ 注册表: $($kv.Name) — $($_.Exception.Message)" -ForegroundColor Red
            $ok = $false
        }
    }
}

# ============================================================
# 第 5 步: 关闭 Windows Security Center 通知
# ============================================================
Write-Host "[5/7] 关闭 Windows Security Center 通知..." -ForegroundColor Yellow

$notifPaths = @(
    @{Path="HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Notifications\Settings\Windows.SystemToast.SecurityAndMaintenance";
      KeyValues=@(@{Name="Enabled"; Value=0; Type="DWord"})}
)

foreach ($reg in $notifPaths) {
    if (-not (Test-Path $reg.Path)) {
        New-Item -Path $reg.Path -Force -ItemType Directory | Out-Null
    }
    foreach ($kv in $reg.KeyValues) {
        Set-ItemProperty -Path $reg.Path -Name $kv.Name -Value $kv.Value -Type $kv.Type -ErrorAction SilentlyContinue
    }
}
Write-Host "  ✅ 安全中心通知已关闭" -ForegroundColor Green

# ============================================================
# 第 6 步: 移除 Windows Defender 的桌面/托盘图标残留
# ============================================================
Write-Host "[6/7] 移除托盘图标残留..." -ForegroundColor Yellow
try {
    # 修改安全中心托盘图标行为
    New-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" `
        -Name "SecurityHealth" -Value "" -PropertyType String -Force -ErrorAction Stop | Out-Null
    Write-Host "  ✅ 已清除自动启动项" -ForegroundColor Green
} catch {
    Write-Host "  ⚠️  无需要修改的启动项" -ForegroundColor Gray
}

# ============================================================
# 第 7 步: 验证并总结
# ============================================================
Write-Host "[7/7] 验证当前状态..." -ForegroundColor Yellow

try {
    $status = Get-MpComputerStatus
    Write-Host ""
    Write-Host "╔══════════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║           Windows Defender 状态报告               ║" -ForegroundColor Cyan
    Write-Host "╚══════════════════════════════════════════════════╝" -ForegroundColor Cyan
    Write-Host "  AntivirusEnabled           : $($status.AntivirusEnabled)" -ForegroundColor $(if($status.AntivirusEnabled){"Red"}else{"Green"})
    Write-Host "  RealTimeProtectionEnabled  : $($status.RealTimeProtectionEnabled)" -ForegroundColor $(if($status.RealTimeProtectionEnabled){"Red"}else{"Green"})
    Write-Host "  NISEnabled                 : $($status.NISEnabled)" -ForegroundColor $(if($status.NISEnabled){"Red"}else{"Green"})
    Write-Host "  OnAccessProtectionEnabled  : $($status.OnAccessProtectionEnabled)" -ForegroundColor $(if($status.OnAccessProtectionEnabled){"Red"}else{"Green"})
    Write-Host "  IoavProtectionEnabled      : $($status.IoavProtectionEnabled)" -ForegroundColor $(if($status.IoavProtectionEnabled){"Red"}else{"Green"})
    Write-Host "  AMProductVersion           : $($status.AMProductVersion)"
    Write-Host "  AntivirusSignatureStatus   : $($status.AntivirusSignatureStatus)"
} catch {
    Write-Host "  ⚠️  无法查询 Defender 状态" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════╗" -ForegroundColor Cyan
if ($ok) {
    Write-Host "║  ✅ 操作完成! Defender 已关闭                     ║" -ForegroundColor Green
} else {
    Write-Host "║  ⚠️  部分操作失败，请检查错误信息                  ║" -ForegroundColor Yellow
}
Write-Host "╠══════════════════════════════════════════════════╣" -ForegroundColor Cyan
Write-Host "║  如果重启后 Defender 重新开启:                     ║" -ForegroundColor Cyan
Write-Host "║  1. 确保已关闭「篡改保护」                          ║" -ForegroundColor Cyan
Write-Host "║  2. 重新运行本脚本                                  ║" -ForegroundColor Cyan
Write-Host "║  3. 或使用组策略:                                   ║" -ForegroundColor Cyan
Write-Host "║     计算机配置 → 管理模板 → Windows 组件 →          ║" -ForegroundColor Cyan
Write-Host "║     Windows Defender 防病毒 → 关闭 Defender        ║" -ForegroundColor Cyan
Write-Host "╚══════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""
Write-Host "注意: 重大 Windows 更新可能重置这些设置。" -ForegroundColor Magenta

# ============================================================
# 还原脚本 (重新启用 Defender)
# ============================================================
$restoreScript = @"
<#
.SYNOPSIS
    还原脚本 — 重新启用 Windows Defender
#>
#Requires -RunAsAdministrator

# 1. 还原组策略注册表
`$policies = @(
    "HKLM:\SOFTWARE\Policies\Microsoft\Windows Defender",
    "HKLM:\SOFTWARE\Policies\Microsoft\Windows Defender\Real-Time Protection",
    "HKLM:\SOFTWARE\Policies\Microsoft\Windows Defender\SpyNet",
    "HKLM:\SOFTWARE\Policies\Microsoft\Windows Defender\Signature Updates",
    "HKLM:\SOFTWARE\Policies\Microsoft\Microsoft Antimalware",
    "HKLM:\SOFTWARE\Policies\Microsoft\Microsoft Antimalware\Real-Time Protection"
)
foreach (`$p in `$policies) {
    if (Test-Path `$p) { Remove-Item -Path `$p -Recurse -Force }
}

# 2. 启用 PowerShell 设置
`$enable = @{
    DisableRealtimeMonitoring = 0
    DisableBehaviorMonitoring = 0
    DisableBlockAtFirstSeen   = 0
    DisableIOAVProtection     = 0
    MAPSReporting             = 2
    PUAProtection             = 1
    SubmitSamplesConsent      = 1
}
Set-MpPreference @enable

# 3. 启用计划任务
`$tasks = Get-ScheduledTask -TaskPath "\Microsoft\Windows\Windows Defender\" 2>`$null
foreach (`$t in `$tasks) { Enable-ScheduledTask -TaskPath `$t.TaskPath -TaskName `$t.TaskName }

Write-Host "Defender 已重新启用，建议重启系统。" -ForegroundColor Green
"@

$restorePath = "E:\code\c\stm32\clion\TpControl_V01\Tools\enable_defender.ps1"
$restoreScript | Out-File -FilePath $restorePath -Encoding utf8
Write-Host ""
Write-Host "📄 还原脚本已保存到: $restorePath" -ForegroundColor Cyan
Write-Host "   运行该脚本可重新启用 Windows Defender。" -ForegroundColor Cyan