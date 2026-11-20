param(
    [string]$LogPath = "C:\Users\Carl\unreal-truss\Saved\Logs\UnrealTruss.log",
    [switch]$AllSessions
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-LogSeconds {
    param([string]$Line)

    if ($Line -match '^\[(\d{4})\.(\d{2})\.(\d{2})-(\d{2})\.(\d{2})\.(\d{2}):(\d{3})\]') {
        return [double]$Matches[4] * 3600.0 +
            [double]$Matches[5] * 60.0 +
            [double]$Matches[6] +
            [double]$Matches[7] / 1000.0
    }

    return $null
}

function Get-Count {
    param(
        [string[]]$Lines,
        [string]$Pattern
    )

    return @($Lines | Select-String -Pattern $Pattern).Count
}

function Format-Rate {
    param(
        [int]$Count,
        [double]$Minutes
    )

    if ($Minutes -le 0.0) {
        return "n/a"
    }

    return "{0:N2}" -f ($Count / $Minutes)
}

function Format-Per100 {
    param(
        [int]$Count,
        [int]$Denominator
    )

    if ($Denominator -le 0) {
        return "n/a"
    }

    return "{0:N1}" -f (($Count * 100.0) / $Denominator)
}

if (!(Test-Path -LiteralPath $LogPath)) {
    throw "Log file not found: $LogPath"
}

$Lines = Get-Content -LiteralPath $LogPath
$NavigationIndices = @()
for ($Index = 0; $Index -lt $Lines.Count; ++$Index) {
    if ($Lines[$Index] -match 'NavigationReady') {
        $NavigationIndices += $Index
    }
}

if ($NavigationIndices.Count -eq 0) {
    $LogDirectory = Split-Path -Parent $LogPath
    $FallbackLog = Get-ChildItem -LiteralPath $LogDirectory -Filter "UnrealTruss*.log" |
        Sort-Object LastWriteTime -Descending |
        Where-Object {
            Select-String -LiteralPath $_.FullName -Pattern 'NavigationReady' -Quiet
        } |
        Select-Object -First 1

    if (!$FallbackLog) {
        throw "No NavigationReady session found in $LogPath or rotated UnrealTruss logs."
    }

    $LogPath = $FallbackLog.FullName
    $Lines = Get-Content -LiteralPath $LogPath
    $NavigationIndices = @()
    for ($Index = 0; $Index -lt $Lines.Count; ++$Index) {
        if ($Lines[$Index] -match 'NavigationReady') {
            $NavigationIndices += $Index
        }
    }
}

$SessionStarts = @()
if ($AllSessions) {
    $SessionStarts = @($NavigationIndices)
}
else {
    $SessionStarts = @($NavigationIndices[-1])
}

foreach ($SessionNumber in 0..($SessionStarts.Count - 1)) {
    $StartIndex = $SessionStarts[$SessionNumber]
    $EndIndex = if ($AllSessions -and $SessionNumber -lt ($SessionStarts.Count - 1)) { $SessionStarts[$SessionNumber + 1] - 1 } else { $Lines.Count - 1 }
    $Segment = $Lines[$StartIndex..$EndIndex]

    $StartSeconds = Get-LogSeconds $Segment[0]
    $EndSeconds = $null
    for ($Index = $Segment.Count - 1; $Index -ge 0; --$Index) {
        $EndSeconds = Get-LogSeconds $Segment[$Index]
        if ($null -ne $EndSeconds) {
            break
        }
    }

    $ElapsedSeconds = if ($null -ne $StartSeconds -and $null -ne $EndSeconds) { $EndSeconds - $StartSeconds } else { 0.0 }
    if ($ElapsedSeconds -lt 0.0) {
        $ElapsedSeconds += 24.0 * 3600.0
    }
    $ElapsedMinutes = $ElapsedSeconds / 60.0

    $Metrics = [ordered]@{
        "WanderStart" = "WanderStart"
        "WanderLimited" = "WanderLimited"
        "TrafficRouteSelected" = "TrafficRouteSelected"
        "TrafficRouteSelected wander" = "TrafficRouteSelected npc=.*reason=wander"
        "SmartObjectClaim" = "SmartObjectClaim"
        "SmartObjectClaim traffic=true" = "SmartObjectClaim .*traffic=true"
        "SmartObjectRelease occupancy_complete" = "SmartObjectRelease npc=.*reason=occupancy_complete"
        "SmartObjectRelease path_idle" = "SmartObjectRelease npc=.*reason=path_idle"
        "WanderExitClearanceStart" = "WanderExitClearanceStart"
        "WanderExitClearanceArrive" = "WanderExitClearanceArrive"
        "WanderAbandon" = "WanderAbandon"
        "NPCMovementStallStart" = "NPCMovementStallStart"
        "NPCMovementStallStart bar/cafe" = "NPCMovementStallStart .*claimed=(CafeTableActor|BarSeatingActor)"
        "NPCMovementDeviationStart" = "NPCMovementDeviationStart"
        "NPCMovementDeviationStart wander" = "NPCMovementDeviationStart .*reason=wander"
        "NPCMovementDeviationStart smart_approach" = "NPCMovementDeviationStart .*reason=smart_approach"
        "NPCMovementDeviationStart smart_slot" = "NPCMovementDeviationStart .*reason=smart_slot"
        "NPCMovementDeviationStart traffic_waypoint" = "NPCMovementDeviationStart .*reason=traffic_waypoint"
        "SmartObjectCandidatesRejected no_through" = "SmartObjectCandidatesRejected .*no_through=[1-9]"
        "WanderCandidatesRejected no_through" = "WanderCandidatesRejected .*no_through=[1-9]"
        "TrafficRouteRejected smart_no_through" = "TrafficRouteRejected .*reason=smart_no_through"
        "TrafficRouteRejected wander_no_through" = "TrafficRouteRejected .*reason=wander_no_through"
        "TrafficWaypointLayerEnabled" = "TrafficWaypointLayerEnabled"
        "blocked_near_target" = "reason=blocked_near_target"
        "move_timeout" = "reason=move_timeout"
        "no_through mentions" = "no_through="
    }

    $Counts = @{}
    foreach ($Metric in $Metrics.GetEnumerator()) {
        $Counts[$Metric.Key] = Get-Count $Segment $Metric.Value
    }

    $WanderStartCount = $Counts["WanderStart"]
    $SmartObjectClaimCount = $Counts["SmartObjectClaim"]

    ""
    "MajicEventAI log summary"
    "Log: $LogPath"
    "Session start: $($Segment[0])"
    "Session length: {0:N1}s ({1:N2} min)" -f $ElapsedSeconds, $ElapsedMinutes
    ""
    "{0,-42} {1,8} {2,10} {3,14} {4,18}" -f "Metric", "Count", "Per min", "Per 100 wander", "Per 100 claims"
    "{0,-42} {1,8} {2,10} {3,14} {4,18}" -f "------", "-----", "-------", "--------------", "--------------"
    foreach ($MetricName in $Metrics.Keys) {
        $Count = $Counts[$MetricName]
        "{0,-42} {1,8} {2,10} {3,14} {4,18}" -f `
            $MetricName,
            $Count,
            (Format-Rate $Count $ElapsedMinutes),
            (Format-Per100 $Count $WanderStartCount),
            (Format-Per100 $Count $SmartObjectClaimCount)
    }

    $StallClaimedCounts = @{}
    foreach ($Match in ($Segment | Select-String -Pattern 'NPCMovementStallStart')) {
        if ($Match.Line -match 'claimed=([^ ]+)') {
            $Key = $Matches[1]
            if (!$StallClaimedCounts.ContainsKey($Key)) {
                $StallClaimedCounts[$Key] = 0
            }
            $StallClaimedCounts[$Key]++
        }
    }

    if ($StallClaimedCounts.Count -gt 0) {
        ""
        "Top stall claimed targets"
        $StallClaimedCounts.GetEnumerator() |
            Sort-Object Value -Descending |
            Select-Object -First 10 |
            ForEach-Object {
                "{0,4}  {1}" -f $_.Value, $_.Key
            }
    }

    $DeviationReasonCounts = @{}
    foreach ($Match in ($Segment | Select-String -Pattern 'NPCMovementDeviationStart')) {
        if ($Match.Line -match 'reason=([^ ]+)') {
            $Key = $Matches[1]
            if (!$DeviationReasonCounts.ContainsKey($Key)) {
                $DeviationReasonCounts[$Key] = 0
            }
            $DeviationReasonCounts[$Key]++
        }
    }

    if ($DeviationReasonCounts.Count -gt 0) {
        ""
        "Deviation reasons"
        $DeviationReasonCounts.GetEnumerator() |
            Sort-Object Value -Descending |
            ForEach-Object {
                "{0,4}  {1}" -f $_.Value, $_.Key
            }
    }

    $DeviationClaimedCounts = @{}
    foreach ($Match in ($Segment | Select-String -Pattern 'NPCMovementDeviationStart')) {
        if ($Match.Line -match 'claimed=([^ ]+)') {
            $Key = $Matches[1]
            if (!$DeviationClaimedCounts.ContainsKey($Key)) {
                $DeviationClaimedCounts[$Key] = 0
            }
            $DeviationClaimedCounts[$Key]++
        }
    }

    if ($DeviationClaimedCounts.Count -gt 0) {
        ""
        "Top deviation claimed targets"
        $DeviationClaimedCounts.GetEnumerator() |
            Sort-Object Value -Descending |
            Select-Object -First 10 |
            ForEach-Object {
                "{0,4}  {1}" -f $_.Value, $_.Key
            }
    }

    $NoThroughAreaCounts = @{}
    foreach ($Match in ($Segment | Select-String -Pattern 'TrafficRouteRejected .*no_through')) {
        if ($Match.Line -match 'area=([^ ]+) count=(\d+)') {
            $Key = $Matches[1]
            $Count = [int]$Matches[2]
            if (!$NoThroughAreaCounts.ContainsKey($Key)) {
                $NoThroughAreaCounts[$Key] = 0
            }
            $NoThroughAreaCounts[$Key] += $Count
        }
    }

    if ($NoThroughAreaCounts.Count -gt 0) {
        ""
        "Top no-through rejected areas"
        $NoThroughAreaCounts.GetEnumerator() |
            Sort-Object Value -Descending |
            Select-Object -First 10 |
            ForEach-Object {
                "{0,4}  {1}" -f $_.Value, $_.Key
            }
    }
}
