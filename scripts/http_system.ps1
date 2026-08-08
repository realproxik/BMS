<#
.SYNOPSIS
Fetch URLs, block links, and rewrite HTML so CSS is loaded before scripts.

.DESCRIPTION
This script accepts URLs via command-line arguments or stdin, fetches each page,
blocks disallowed links, and rewrites HTML so stylesheet tags appear before
script tags.

.EXAMPLE
powershell -ExecutionPolicy Bypass -File scripts\http_system.ps1 https://example.com
Get-Content urls.txt | powershell -ExecutionPolicy Bypass -File scripts\http_system.ps1 -FromStdin
#>

param(
    [switch]$FromStdin,
    [string[]]$Urls,
    [string]$OutputDir = ".",
    [string]$BlockList = "tracker.example.com,bad.com",
    [switch]$Verbose
)

function Get-BlockedPatterns {
    param([string]$list)
    return $list -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' }
}

function Is-BlockedUrl {
    param([string]$url, [string[]]$patterns)
    foreach ($pattern in $patterns) {
        if ($url -match [regex]::Escape($pattern)) {
            return $true
        }
    }
    return $false
}

function Rewrite-HTML {
    param([string]$html, [string[]]$blockPatterns)

    $cssTags = [regex]::Matches($html, '<link[^>]+rel\s*=\s*("|\')?stylesheet\1[^>]*>', 'IgnoreCase') |
        ForEach-Object { $_.Value }
    $scriptTags = [regex]::Matches($html, '<script[^>]*>.*?</script>', 'Singleline,IgnoreCase') |
        ForEach-Object { $_.Value }

    $html = [regex]::Replace($html, '<link[^>]+rel\s*=\s*("|\')?stylesheet\1[^>]*>', '', 'IgnoreCase')
    $html = [regex]::Replace($html, '<script[^>]*>.*?</script>', '', 'Singleline,IgnoreCase')

    $html = [regex]::Replace($html, '<a([^>]+?)href\s*=\s*("|\')([^"\']+)("|\')', {
        param($m)
        $href = $m.Groups[3].Value
        if ($href -match '^javascript:' -or $href -match '^data:' -or (Is-BlockedUrl -url $href -patterns $blockPatterns)) {
            return "<a$($m.Groups[1].Value)href=\"#blocked\""
        }
        return $m.Value
    }, 'IgnoreCase')

    if ($cssTags.Count -gt 0) {
        $cssBlock = $cssTags -join "`n"
        if ($html -match '<head[^>]*>') {
            $html = [regex]::Replace($html, '(<head[^>]*>)', "$1`n$cssBlock", 'IgnoreCase')
        } else {
            $html = "$cssBlock`n$html"
        }
    }

    if ($scriptTags.Count -gt 0) {
        $scriptBlock = $scriptTags -join "`n"
        if ($html -match '</body>') {
            $html = [regex]::Replace($html, '</body>', "$scriptBlock`n</body>", 'IgnoreCase')
        } else {
            $html += "`n$scriptBlock"
        }
    }

    return $html
}

if ($FromStdin) {
    $stdinUrls = @()
    while ($line = [Console]::In.ReadLine()) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        $stdinUrls += $line.Trim()
    }
    if ($stdinUrls.Count -gt 0) {
        $Urls = @($Urls + $stdinUrls)
    }
}

if (-not $Urls -or $Urls.Count -eq 0) {
    Write-Host "Usage: powershell -File scripts\http_system.ps1 [-FromStdin] [-Urls <url1> <url2>] [-OutputDir <dir>] [-BlockList <pattern1,pattern2>]"
    exit 1
}

$blockPatterns = Get-BlockedPatterns -list $BlockList
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$count = 0
foreach ($url in $Urls) {
    $count++
    if (Is-BlockedUrl -url $url -patterns $blockPatterns) {
        Write-Warning "Blocked URL: $url"
        continue
    }

    try {
        if ($Verbose) { Write-Host "Fetching: $url" }
        $response = Invoke-WebRequest -Uri $url -UseBasicParsing -ErrorAction Stop
        $html = $response.Content
    } catch {
        Write-Warning "Failed to fetch $url: $_"
        continue
    }

    $rewritten = Rewrite-HTML -html $html -blockPatterns $blockPatterns
    $fileName = "output_$($count).html"
    $outputPath = Join-Path $OutputDir $fileName
    Set-Content -Path $outputPath -Value $rewritten -Encoding UTF8
    Write-Host "Saved rewritten page to $outputPath"
}
