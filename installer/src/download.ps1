# ============================================================================
# download_system.ps1 - BMS Browser Download System
# Version: 1.0.0
# Description: Multi-threaded download system with API processing, ignore patterns,
#              package management, and object handling
# ============================================================================

#Requires -Version 5.1

# ============================================================================
# Configuration Section
# ============================================================================

$script:DownloadConfig = @{
    MaxConcurrentDownloads = 4
    MaxRetries = 3
    RetryDelayMs = 1000
    TimeoutSeconds = 300
    BufferSize = 8192
    VerifyChecksum = $true
    ExtractArchives = $true
    ResumePartial = $true
    UserAgent = "BMS-Browser/1.0"
    TempDirectory = "$env:TEMP\BMS_Downloads"
    DownloadDirectory = "C:\Users\DeLL\Desktop\BMS\build\dependencies"
    LogDirectory = "C:\Users\DeLL\Desktop\BMS\logs"
    IgnoreFile = ".bmsignore"
    ManifestFile = "manifest.json"
    ChunkSize = 1048576  # 1MB chunks for resumable downloads
}

# ============================================================================
# Enumerations
# ============================================================================

Add-Type -TypeDefinition @"
    public enum DownloadStatus {
        PENDING = 0,
        QUEUED = 1,
        DOWNLOADING = 2,
        PAUSED = 3,
        COMPLETED = 4,
        FAILED = 5,
        CANCELLED = 6,
        VERIFYING = 7,
        EXTRACTING = 8,
        RETRYING = 9,
        IGNORED = 10
    }
    
    public enum DownloadPriority {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3
    }
    
    public enum DownloadType {
        API = 0,
        PACKAGE = 1,
        OBJECT = 2,
        DEPENDENCY = 3,
        SOURCE = 4,
        BINARY = 5,
        ASSET = 6,
        CONFIGURATION = 7,
        IGNORE_FILE = 8,
        MANIFEST = 9
    }
    
    public enum DownloadProtocol {
        HTTP = 0,
        HTTPS = 1,
        FTP = 2,
        SFTP = 3,
        GIT = 4,
        SVN = 5,
        MERCURIAL = 6,
        LOCAL = 7
    }
"@

# ============================================================================
# Data Structures
# ============================================================================

class DownloadItem {
    [string]$Id
    [string]$Url
    [string]$Destination
    [string]$FileName
    [DownloadStatus]$Status
    [DownloadPriority]$Priority
    [DownloadType]$Type
    [DownloadProtocol]$Protocol
    [long]$TotalSize
    [long]$DownloadedSize
    [int]$RetryCount
    [string]$Checksum
    [string]$ChecksumType
    [hashtable]$Headers
    [hashtable]$Metadata
    [datetime]$StartTime
    [datetime]$EndTime
    [string]$Error
    [string]$Progress
    [bool]$Resumable
    [int]$ThreadId
    [string]$TempFile
    [string]$MirrorUrl
    
    DownloadItem() {
        $this.Id = [System.Guid]::NewGuid().ToString()
        $this.Status = [DownloadStatus]::PENDING
        $this.Priority = [DownloadPriority]::NORMAL
        $this.Headers = @{}
        $this.Metadata = @{}
        $this.RetryCount = 0
        $this.Resumable = $true
        $this.ThreadId = -1
    }
    
    [string] ToString() {
        return "[$($this.Status)] $($this.FileName) ($([math]::Round($this.DownloadedSize/1MB, 2))MB/$([math]::Round($this.TotalSize/1MB, 2))MB) - $($this.Url)"
    }
}

class IgnorePattern {
    [string]$Pattern
    [bool]$IsRegex
    [bool]$IsNegated
    [string]$Category
    [string]$Description
    
    IgnorePattern() {
        $this.IsRegex = $false
        $this.IsNegated = $false
    }
}

class Package {
    [string]$Name
    [string]$Version
    [string]$Source
    [string]$Destination
    [string]$Checksum
    [string]$ChecksumType
    [string[]]$Dependencies
    [string[]]$Includes
    [string[]]$Excludes
    [hashtable]$Metadata
    [bool]$Installed
    [datetime]$InstallDate
    [string]$InstallPath
    
    Package() {
        $this.Metadata = @{}
        $this.Installed = $false
        $this.Dependencies = @()
        $this.Includes = @()
        $this.Excludes = @()
    }
}

class ApiEndpoint {
    [string]$Name
    [string]$Url
    [string]$Version
    [string]$Method
    [string]$Auth
    [hashtable]$Headers
    [hashtable]$Parameters
    [string]$ResponseType
    [int]$Timeout
    [bool]$Enabled
    
    ApiEndpoint() {
        $this.Headers = @{}
        $this.Parameters = @{}
        $this.Method = "GET"
        $this.Timeout = 30
        $this.Enabled = $true
    }
}

# ============================================================================
# Core Download Manager Class
# ============================================================================

class DownloadManager {
    # Private fields
    hidden [hashtable] $_config
    hidden [System.Collections.Generic.List[DownloadItem]] $_queue
    hidden [System.Collections.Generic.List[DownloadItem]] $_activeDownloads
    hidden [System.Collections.Generic.List[DownloadItem]] $_completedDownloads
    hidden [System.Collections.Generic.List[IgnorePattern]] $_ignorePatterns
    hidden [System.Collections.Generic.List[Package]] $_packages
    hidden [System.Collections.Generic.List[ApiEndpoint]] $_apiEndpoints
    hidden [hashtable] $_objects
    hidden [hashtable] $_downloadedFiles
    hidden [System.Collections.Generic.Dictionary[string, hashtable]] $_fileRegistry
    
    hidden [object] $_lock
    hidden [System.Threading.Tasks.Task] $_downloadTask
    hidden [System.Threading.CancellationTokenSource] $_cancellationTokenSource
    hidden [bool] $_isRunning
    hidden [int] $_activeThreads
    hidden [int] $_totalDownloads
    hidden [int] $_completedCount
    hidden [int] $_failedCount
    hidden [int] $_ignoredCount
    
    # Events
    [scriptblock]$OnDownloadStart
    [scriptblock]$OnDownloadProgress
    [scriptblock]$OnDownloadComplete
    [scriptblock]$OnDownloadFailed
    [scriptblock]$OnDownloadCancelled
    [scriptblock]$OnAllComplete
    
    # Constructor
    DownloadManager([hashtable]$config = $null) {
        $this._config = if ($config) { $config } else { $script:DownloadConfig }
        $this._queue = [System.Collections.Generic.List[DownloadItem]]::new()
        $this._activeDownloads = [System.Collections.Generic.List[DownloadItem]]::new()
        $this._completedDownloads = [System.Collections.Generic.List[DownloadItem]]::new()
        $this._ignorePatterns = [System.Collections.Generic.List[IgnorePattern]]::new()
        $this._packages = [System.Collections.Generic.List[Package]]::new()
        $this._apiEndpoints = [System.Collections.Generic.List[ApiEndpoint]]::new()
        $this._objects = @{}
        $this._downloadedFiles = @{}
        $this._fileRegistry = [System.Collections.Generic.Dictionary[string, hashtable]]::new()
        $this._lock = [object]::new()
        $this._cancellationTokenSource = [System.Threading.CancellationTokenSource]::new()
        $this._isRunning = $false
        $this._activeThreads = 0
        $this._totalDownloads = 0
        $this._completedCount = 0
        $this._failedCount = 0
        $this._ignoredCount = 0
        
        # Create directories
        $this._CreateDirectories()
        
        # Load ignore patterns
        $this._LoadIgnorePatterns()
        
        # Load manifest
        $this._LoadManifest()
    }
    
    # ========================================================================
    # Public Methods
    # ========================================================================
    
    [void] AddDownload([DownloadItem]$item) {
        $this._queue.Add($item)
        $this._totalDownloads++
        Write-Host "[DownloadManager] Added: $($item.FileName)" -ForegroundColor Cyan
    }
    
    [void] AddDownloads([DownloadItem[]]$items) {
        foreach ($item in $items) {
            $this.AddDownload($item)
        }
    }
    
    [void] AddApiEndpoint([ApiEndpoint]$endpoint) {
        $this._apiEndpoints.Add($endpoint)
        Write-Host "[DownloadManager] API Endpoint added: $($endpoint.Name)" -ForegroundColor Green
    }
    
    [void] AddApiEndpoints([ApiEndpoint[]]$endpoints) {
        foreach ($endpoint in $endpoints) {
            $this.AddApiEndpoint($endpoint)
        }
    }
    
    [void] AddPackage([Package]$package) {
        $this._packages.Add($package)
        Write-Host "[DownloadManager] Package added: $($package.Name) v$($package.Version)" -ForegroundColor Green
    }
    
    [void] AddPackages([Package[]]$packages) {
        foreach ($package in $packages) {
            $this.AddPackage($package)
        }
    }
    
    [void] AddIgnorePattern([IgnorePattern]$pattern) {
        $this._ignorePatterns.Add($pattern)
        Write-Host "[DownloadManager] Ignore pattern added: $($pattern.Pattern)" -ForegroundColor Yellow
    }
    
    [void] AddObject([string]$name, [hashtable]$obj) {
        $this._objects[$name] = $obj
    }
    
    [hashtable] GetObject([string]$name) {
        return $this._objects[$name]
    }
    
    [void] Start() {
        if ($this._isRunning) {
            Write-Warning "[DownloadManager] Already running"
            return
        }
        
        $this._isRunning = $true
        Write-Host "[DownloadManager] Starting download system..." -ForegroundColor Green
        
        # Process APIs first
        $this._ProcessApis()
        
        # Start download task
        $this._downloadTask = [System.Threading.Tasks.Task]::Run({ $this._DownloadLoop() })
    }
    
    [void] Stop() {
        if (-not $this._isRunning) {
            return
        }
        
        Write-Host "[DownloadManager] Stopping download system..." -ForegroundColor Yellow
        $this._cancellationTokenSource.Cancel()
        $this._isRunning = $false
        
        if ($this._downloadTask) {
            $this._downloadTask.Wait(10000)
        }
        
        Write-Host "[DownloadManager] Stopped" -ForegroundColor Yellow
    }
    
    [void] Pause() {
        Write-Host "[DownloadManager] Pausing downloads..." -ForegroundColor Yellow
        $this._isRunning = $false
    }
    
    [void] Resume() {
        Write-Host "[DownloadManager] Resuming downloads..." -ForegroundColor Green
        $this._isRunning = $true
        $this._cancellationTokenSource = [System.Threading.CancellationTokenSource]::new()
        $this._downloadTask = [System.Threading.Tasks.Task]::Run({ $this._DownloadLoop() })
    }
    
    [void] Cancel() {
        Write-Host "[DownloadManager] Cancelling all downloads..." -ForegroundColor Red
        $this._cancellationTokenSource.Cancel()
        $this._queue.Clear()
        $this._isRunning = $false
    }
    
    [void] ClearQueue() {
        Write-Host "[DownloadManager] Clearing queue..." -ForegroundColor Yellow
        $this._queue.Clear()
    }
    
    [string] GetStatus() {
        return @"
╔═══════════════════════════════════════════════════════════════════╗
║                    DOWNLOAD SYSTEM STATUS                        ║
╠═══════════════════════════════════════════════════════════════════╣
║ Status:                    $($this._isRunning ? 'Running' : 'Stopped')
║ Total Downloads:           $($this._totalDownloads)
║ Completed:                 $($this._completedCount)
║ Failed:                    $($this._failedCount)
║ Ignored:                   $($this._ignoredCount)
║ Active Downloads:          $($this._activeDownloads.Count)
║ Queued Downloads:          $($this._queue.Count)
║ Active Threads:            $($this._activeThreads)
║ Packages:                  $($this._packages.Count)
║ API Endpoints:             $($this._apiEndpoints.Count)
║ Ignore Patterns:           $($this._ignorePatterns.Count)
╚═══════════════════════════════════════════════════════════════════╝
"@
    }
    
    [hashtable] GetStatistics() {
        return @{
            TotalDownloads = $this._totalDownloads
            CompletedCount = $this._completedCount
            FailedCount = $this._failedCount
            IgnoredCount = $this._ignoredCount
            ActiveCount = $this._activeDownloads.Count
            QueuedCount = $this._queue.Count
            ActiveThreads = $this._activeThreads
            PackageCount = $this._packages.Count
            ApiCount = $this._apiEndpoints.Count
            IgnoreCount = $this._ignorePatterns.Count
            IsRunning = $this._isRunning
            DownloadSpeed = $this._GetAverageSpeed()
        }
    }
    
    [void] ExportState([string]$path) {
        $state = @{
            Queue = $this._queue | ForEach-Object { $this._ExportDownloadItem($_) }
            Completed = $this._completedDownloads | ForEach-Object { $this._ExportDownloadItem($_) }
            Packages = $this._packages | ForEach-Object { $this._ExportPackage($_) }
            Config = $this._config
            Statistics = $this.GetStatistics()
        }
        $state | ConvertTo-Json -Depth 10 | Set-Content -Path $path -Encoding UTF8
        Write-Host "[DownloadManager] State exported to: $path" -ForegroundColor Green
    }
    
    [void] ImportState([string]$path) {
        if (-not (Test-Path $path)) {
            Write-Warning "[DownloadManager] State file not found: $path"
            return
        }
        $state = Get-Content -Path $path -Raw | ConvertFrom-Json
        Write-Host "[DownloadManager] State imported from: $path" -ForegroundColor Green
    }
    
    # ========================================================================
    # Private Methods
    # ========================================================================
    
    hidden [void] _CreateDirectories() {
        $dirs = @(
            $this._config.TempDirectory,
            $this._config.DownloadDirectory,
            $this._config.LogDirectory
        )
        foreach ($dir in $dirs) {
            if (-not (Test-Path $dir)) {
                New-Item -ItemType Directory -Path $dir -Force | Out-Null
                Write-Host "[DownloadManager] Created directory: $dir" -ForegroundColor Gray
            }
        }
    }
    
    hidden [void] _LoadIgnorePatterns() {
        $ignorePath = Join-Path $this._config.DownloadDirectory $this._config.IgnoreFile
        if (Test-Path $ignorePath) {
            Write-Host "[DownloadManager] Loading ignore patterns from: $ignorePath" -ForegroundColor Gray
            $lines = Get-Content -Path $ignorePath
            foreach ($line in $lines) {
                if ($line -and -not $line.StartsWith('#')) {
                    $pattern = [IgnorePattern]::new()
                    if ($line.StartsWith('!')) {
                        $pattern.IsNegated = $true
                        $pattern.Pattern = $line.Substring(1).Trim()
                    } else {
                        $pattern.Pattern = $line.Trim()
                    }
                    $pattern.IsRegex = $pattern.Pattern -match '[*?\[\]]'
                    $this._ignorePatterns.Add($pattern)
                }
            }
            Write-Host "[DownloadManager] Loaded $($this._ignorePatterns.Count) ignore patterns" -ForegroundColor Gray
        }
    }
    
    hidden [void] _LoadManifest() {
        $manifestPath = Join-Path $this._config.DownloadDirectory $this._config.ManifestFile
        if (Test-Path $manifestPath) {
            Write-Host "[DownloadManager] Loading manifest from: $manifestPath" -ForegroundColor Gray
            $manifest = Get-Content -Path $manifestPath -Raw | ConvertFrom-Json
            # Process manifest
        }
    }
    
    hidden [void] _ProcessApis() {
        Write-Host "[DownloadManager] Processing API endpoints..." -ForegroundColor Cyan
        
        foreach ($api in $this._apiEndpoints) {
            if (-not $api.Enabled) { continue }
            
            try {
                Write-Host "[DownloadManager] Calling API: $($api.Name)" -ForegroundColor Gray
                $response = $this._CallApi($api)
                $this._ProcessApiResponse($api, $response)
            } catch {
                Write-Host "[DownloadManager] API call failed: $($api.Name) - $_" -ForegroundColor Red
            }
        }
    }
    
    hidden [hashtable] _CallApi([ApiEndpoint]$api) {
        $params = @{
            Uri = $api.Url
            Method = $api.Method
            Headers = $api.Headers
            TimeoutSec = $api.Timeout
            UseBasicParsing = $true
        }
        
        if ($api.Parameters.Count -gt 0) {
            $params.Body = $api.Parameters | ConvertTo-Json
            $params.ContentType = "application/json"
        }
        
        $response = Invoke-WebRequest @params
        return @{
            Content = $response.Content
            StatusCode = $response.StatusCode
            Headers = $response.Headers
        }
    }
    
    hidden [void] _ProcessApiResponse([ApiEndpoint]$api, [hashtable]$response) {
        if ($response.StatusCode -eq 200) {
            Write-Host "[DownloadManager] API success: $($api.Name)" -ForegroundColor Green
            
            # Parse response
            try {
                $data = $response.Content | ConvertFrom-Json
                
                # Process based on response type
                switch ($api.ResponseType) {
                    "downloads" {
                        $this._ProcessDownloadsFromApi($data)
                    }
                    "packages" {
                        $this._ProcessPackagesFromApi($data)
                    }
                    "objects" {
                        $this._ProcessObjectsFromApi($data)
                    }
                    "manifest" {
                        $this._ProcessManifestFromApi($data)
                    }
                    default {
                        Write-Host "[DownloadManager] Unknown response type: $($api.ResponseType)" -ForegroundColor Yellow
                    }
                }
            } catch {
                Write-Host "[DownloadManager] Failed to parse API response: $_" -ForegroundColor Red
            }
        } else {
            Write-Host "[DownloadManager] API error: $($api.Name) - Status: $($response.StatusCode)" -ForegroundColor Red
        }
    }
    
    hidden [void] _ProcessDownloadsFromApi($data) {
        Write-Host "[DownloadManager] Processing downloads from API..." -ForegroundColor Cyan
        
        foreach ($item in $data) {
            $download = [DownloadItem]::new()
            $download.Url = $item.url
            $download.FileName = $item.filename
            $download.Destination = $item.destination
            $download.Type = [DownloadType]$item.type
            $download.Priority = [DownloadPriority]$item.priority
            $download.Checksum = $item.checksum
            $download.ChecksumType = $item.checksum_type
            $download.Headers = $item.headers
            $download.Metadata = $item.metadata
            
            $this.AddDownload($download)
        }
    }
    
    hidden [void] _ProcessPackagesFromApi($data) {
        Write-Host "[DownloadManager] Processing packages from API..." -ForegroundColor Cyan
        
        foreach ($pkg in $data) {
            $package = [Package]::new()
            $package.Name = $pkg.name
            $package.Version = $pkg.version
            $package.Source = $pkg.source
            $package.Destination = $pkg.destination
            $package.Checksum = $pkg.checksum
            $package.ChecksumType = $pkg.checksum_type
            $package.Dependencies = $pkg.dependencies
            $package.Includes = $pkg.includes
            $package.Excludes = $pkg.excludes
            $package.Metadata = $pkg.metadata
            
            $this.AddPackage($package)
            
            # Process dependencies
            foreach ($dep in $package.Dependencies) {
                $this._ProcessDependency($dep)
            }
        }
    }
    
    hidden [void] _ProcessObjectsFromApi($data) {
        Write-Host "[DownloadManager] Processing objects from API..." -ForegroundColor Cyan
        
        foreach ($obj in $data) {
            $this.AddObject($obj.name, $obj.data)
        }
    }
    
    hidden [void] _ProcessManifestFromApi($data) {
        Write-Host "[DownloadManager] Processing manifest from API..." -ForegroundColor Cyan
        # Process manifest data
    }
    
    hidden [void] _ProcessDependency([string]$dependency) {
        Write-Host "[DownloadManager] Processing dependency: $dependency" -ForegroundColor Gray
        # Download dependency
    }
    
    hidden [void] _DownloadLoop() {
        Write-Host "[DownloadManager] Download loop started" -ForegroundColor Green
        
        while ($this._isRunning -and -not $this._cancellationTokenSource.Token.IsCancellationRequested) {
            try {
                # Check if we can start new downloads
                if ($this._activeDownloads.Count -ge $this._config.MaxConcurrentDownloads) {
                    Start-Sleep -Milliseconds 100
                    continue
                }
                
                # Get next item from queue
                $item = $this._GetNextItem()
                if (-not $item) {
                    # No items in queue
                    if ($this._activeDownloads.Count -eq 0) {
                        # All done
                        $this._isRunning = $false
                        if ($this.OnAllComplete) {
                            & $this.OnAllComplete
                        }
                        break
                    }
                    Start-Sleep -Milliseconds 100
                    continue
                }
                
                # Check if item should be ignored
                if ($this._ShouldIgnore($item)) {
                    Write-Host "[DownloadManager] Ignoring: $($item.FileName)" -ForegroundColor Gray
                    $this._ignoredCount++
                    $item.Status = [DownloadStatus]::IGNORED
                    $this._completedDownloads.Add($item)
                    continue
                }
                
                # Start download
                $this._StartDownload($item)
                
            } catch {
                Write-Host "[DownloadManager] Download loop error: $_" -ForegroundColor Red
                Start-Sleep -Milliseconds 1000
            }
        }
        
        Write-Host "[DownloadManager] Download loop ended" -ForegroundColor Yellow
    }
    
    hidden [DownloadItem] _GetNextItem() {
        $lock = $this._lock
        [System.Threading.Monitor]::Enter($lock)
        try {
            if ($this._queue.Count -eq 0) {
                return $null
            }
            
            # Sort by priority
            $sorted = $this._queue | Sort-Object { $_.Priority.value__ } -Descending
            $item = $sorted[0]
            $this._queue.Remove($item)
            return $item
        } finally {
            [System.Threading.Monitor]::Exit($lock)
        }
    }
    
    hidden [bool] _ShouldIgnore([DownloadItem]$item) {
        foreach ($pattern in $this._ignorePatterns) {
            $match = $false
            if ($pattern.IsRegex) {
                $match = $item.FileName -match $pattern.Pattern
            } else {
                $match = $item.FileName -like $pattern.Pattern
            }
            
            if ($pattern.IsNegated -and $match) {
                return $false
            } elseif ($match) {
                return $true
            }
        }
        return $false
    }
    
    hidden [void] _StartDownload([DownloadItem]$item) {
        $item.Status = [DownloadStatus]::DOWNLOADING
        $item.StartTime = Get-Date
        $item.ThreadId = [System.Threading.Thread]::CurrentThread.ManagedThreadId
        
        $this._activeDownloads.Add($item)
        
        Write-Host "[DownloadManager] Starting download: $($item.FileName) (Thread $($item.ThreadId))" -ForegroundColor Cyan
        
        # Start download in background
        $task = [System.Threading.Tasks.Task]::Run({
            $this._DoDownload($item)
        })
        
        # Register completion callback
        $null = $task.ContinueWith({
            param($t)
            $this._OnDownloadComplete($item)
        })
    }
    
    hidden [void] _DoDownload([DownloadItem]$item) {
        try {
            # Determine protocol
            $protocol = $this._GetProtocol($item.Url)
            
            switch ($protocol) {
                "git" { $this._DownloadGit($item) }
                "http" { $this._DownloadHttp($item) }
                "ftp" { $this._DownloadFtp($item) }
                "local" { $this._DownloadLocal($item) }
                default { throw "Unsupported protocol: $protocol" }
            }
            
            $item.Status = [DownloadStatus]::COMPLETED
            $item.EndTime = Get-Date
            $this._completedCount++
            
            if ($this.OnDownloadComplete) {
                & $this.OnDownloadComplete $item
            }
            
        } catch {
            $item.Status = [DownloadStatus]::FAILED
            $item.Error = $_.Exception.Message
            $item.EndTime = Get-Date
            $this._failedCount++
            
            Write-Host "[DownloadManager] Download failed: $($item.FileName) - $($item.Error)" -ForegroundColor Red
            
            if ($this.OnDownloadFailed) {
                & $this.OnDownloadFailed $item
            }
            
            # Retry if possible
            if ($item.RetryCount -lt $this._config.MaxRetries) {
                $item.RetryCount++
                $item.Status = [DownloadStatus]::RETRYING
                Write-Host "[DownloadManager] Retrying: $($item.FileName) (Attempt $($item.RetryCount))" -ForegroundColor Yellow
                Start-Sleep -Milliseconds $this._config.RetryDelayMs
                $this._queue.Add($item)
            }
        }
    }
    
    hidden [string] _GetProtocol([string]$url) {
        if ($url -match '^git@|\.git$|github\.com') {
            return "git"
        } elseif ($url -match '^https?://') {
            return "http"
        } elseif ($url -match '^ftp://') {
            return "ftp"
        } elseif ($url -match '^file://|^[A-Za-z]:\\') {
            return "local"
        } else {
            return "http"
        }
    }
    
    hidden [void] _DownloadHttp([DownloadItem]$item) {
        $destination = $this._GetDestinationPath($item)
        $tempFile = Join-Path $this._config.TempDirectory "$($item.Id).tmp"
        $item.TempFile = $tempFile
        
        # Create web client
        $webClient = [System.Net.WebClient]::new()
        $webClient.Headers.Add("User-Agent", $this._config.UserAgent)
        
        # Add custom headers
        foreach ($key in $item.Headers.Keys) {
            $webClient.Headers.Add($key, $item.Headers[$key])
        }
        
        # Handle resume
        if ($this._config.ResumePartial -and (Test-Path $tempFile)) {
            $fileInfo = Get-Item $tempFile
            if ($fileInfo.Length -gt 0) {
                $webClient.Headers.Add("Range", "bytes=$($fileInfo.Length)-")
                $item.DownloadedSize = $fileInfo.Length
                Write-Host "[DownloadManager] Resuming download: $($item.FileName) at $([math]::Round($item.DownloadedSize/1MB, 2))MB" -ForegroundColor Yellow
            }
        }
        
        # Setup progress event
        $event = Register-ObjectEvent -InputObject $webClient -EventName DownloadProgressChanged -Action {
            $sender = $EventArgs.UserState
            $progress = $EventArgs.ProgressPercentage
            $total = $EventArgs.TotalBytesToReceive
            $received = $EventArgs.BytesReceived
            
            if ($sender.OnDownloadProgress) {
                & $sender.OnDownloadProgress @{
                    Item = $sender
                    Progress = $progress
                    Total = $total
                    Received = $received
                }
            }
        }
        
        try {
            # Download file
            $webClient.DownloadFile($item.Url, $tempFile)
            
            # Move to destination
            if (Test-Path $destination) {
                Remove-Item $destination -Force
            }
            Move-Item $tempFile $destination -Force
            
            $item.DownloadedSize = (Get-Item $destination).Length
            $item.TotalSize = $item.DownloadedSize
            
        } finally {
            Unregister-Event -SubscriptionId $event.Id
            $webClient.Dispose()
        }
    }
    
    hidden [void] _DownloadGit([DownloadItem]$item) {
        Write-Host "[DownloadManager] Git clone: $($item.Url)" -ForegroundColor Cyan
        
        $destination = $this._GetDestinationPath($item)
        $depthArg = if ($item.Metadata.ContainsKey('Depth')) { "--depth $($item.Metadata.Depth)" } else { "" }
        
        $gitCmd = "git clone $depthArg $($item.Url) `"$destination`""
        Write-Host "[DownloadManager] Running: $gitCmd" -ForegroundColor Gray
        
        $process = Start-Process -FilePath "git" -ArgumentList "clone $depthArg $($item.Url) `"$destination`"" -Wait -PassThru -NoNewWindow
        
        if ($process.ExitCode -ne 0) {
            throw "Git clone failed with exit code: $($process.ExitCode)"
        }
        
        # Get repository size
        if (Test-Path $destination) {
            $size = (Get-ChildItem $destination -Recurse -File | Measure-Object -Property Length -Sum).Sum
            $item.TotalSize = $size
            $item.DownloadedSize = $size
        }
    }
    
    hidden [void] _DownloadFtp([DownloadItem]$item) {
        throw "FTP download not implemented yet"
    }
    
    hidden [void] _DownloadLocal([DownloadItem]$item) {
        Write-Host "[DownloadManager] Local copy: $($item.Url)" -ForegroundColor Cyan
        
        $source = $item.Url -replace '^file://', ''
        $destination = $this->_GetDestinationPath($item)
        
        if (-not (Test-Path $source)) {
            throw "Local file not found: $source"
        }
        
        # Copy file
        Copy-Item -Path $source -Destination $destination -Force
        
        $item.TotalSize = (Get-Item $destination).Length
        $item.DownloadedSize = $item.TotalSize
    }
    
    hidden [string] _GetDestinationPath([DownloadItem]$item) {
        if ($item.Destination) {
            $path = $item.Destination
        } else {
            $path = $this._config.DownloadDirectory
        }
        
        if ($item.FileName) {
            if (Test-Path $path -PathType Container) {
                return Join-Path $path $item.FileName
            } else {
                return $path
            }
        }
        
        return $path
    }
    
    hidden [void] _OnDownloadComplete([DownloadItem]$item) {
        $lock = $this._lock
        [System.Threading.Monitor]::Enter($lock)
        try {
            $this._activeDownloads.Remove($item)
            $this._completedDownloads.Add($item)
            
            # Register downloaded file
            $this._fileRegistry[$item.Id] = @{
                FileName = $item.FileName
                Path = $this._GetDestinationPath($item)
                Size = $item.TotalSize
                Checksum = $item.Checksum
                Type = $item.Type
                Downloaded = $item.EndTime
            }
            
            Write-Host "[DownloadManager] Completed: $($item.FileName) ($([math]::Round($item.TotalSize/1MB, 2))MB)" -ForegroundColor Green
            
        } finally {
            [System.Threading.Monitor]::Exit($lock)
        }
    }
    
    hidden [int] _GetAverageSpeed() {
        return 0  # Placeholder
    }
    
    hidden [hashtable] _ExportDownloadItem([DownloadItem]$item) {
        return @{
            Id = $item.Id
            Url = $item.Url
            FileName = $item.FileName
            Status = $item.Status.ToString()
            Priority = $item.Priority.ToString()
            Type = $item.Type.ToString()
            TotalSize = $item.TotalSize
            DownloadedSize = $item.DownloadedSize
            RetryCount = $item.RetryCount
            Checksum = $item.Checksum
            ChecksumType = $item.ChecksumType
            Headers = $item.Headers
            Metadata = $item.Metadata
            StartTime = $item.StartTime
            EndTime = $item.EndTime
            Error = $item.Error
            Resumable = $item.Resumable
        }
    }
    
    hidden [hashtable] _ExportPackage([Package]$package) {
        return @{
            Name = $package.Name
            Version = $package.Version
            Source = $package.Source
            Destination = $package.Destination
            Checksum = $package.Checksum
            ChecksumType = $package.ChecksumType
            Dependencies = $package.Dependencies
            Includes = $package.Includes
            Excludes = $package.Excludes
            Metadata = $package.Metadata
            Installed = $package.Installed
            InstallDate = $package.InstallDate
            InstallPath = $package.InstallPath
        }
    }
}

# ============================================================================
# Helper Functions
# ============================================================================

function New-DownloadItem {
    param(
        [string]$Url,
        [string]$Destination,
        [string]$FileName,
        [DownloadPriority]$Priority = [DownloadPriority]::NORMAL,
        [DownloadType]$Type = [DownloadType]::BINARY,
        [string]$Checksum = "",
        [string]$ChecksumType = "SHA256",
        [hashtable]$Headers = @{},
        [hashtable]$Metadata = @{}
    )
    
    $item = [DownloadItem]::new()
    $item.Url = $Url
    $item.Destination = $Destination
    $item.FileName = if ($FileName) { $FileName } else { [System.IO.Path]::GetFileName($Url) }
    $item.Priority = $Priority
    $item.Type = $Type
    $item.Checksum = $Checksum
    $item.ChecksumType = $ChecksumType
    $item.Headers = $Headers
    $item.Metadata = $Metadata
    
    return $item
}

function New-Package {
    param(
        [string]$Name,
        [string]$Version,
        [string]$Source,
        [string]$Destination,
        [string]$Checksum = "",
        [string]$ChecksumType = "SHA256",
        [string[]]$Dependencies = @(),
        [string[]]$Includes = @(),
        [string[]]$Excludes = @(),
        [hashtable]$Metadata = @{}
    )
    
    $package = [Package]::new()
    $package.Name = $Name
    $package.Version = $Version
    $package.Source = $Source
    $package.Destination = $Destination
    $package.Checksum = $Checksum
    $package.ChecksumType = $ChecksumType
    $package.Dependencies = $Dependencies
    $package.Includes = $Includes
    $package.Excludes = $Excludes
    $package.Metadata = $Metadata
    
    return $package
}

function New-ApiEndpoint {
    param(
        [string]$Name,
        [string]$Url,
        [string]$Version = "1.0",
        [string]$Method = "GET",
        [string]$Auth = "",
        [hashtable]$Headers = @{},
        [hashtable]$Parameters = @{},
        [string]$ResponseType = "",
        [int]$Timeout = 30,
        [bool]$Enabled = $true
    )
    
    $api = [ApiEndpoint]::new()
    $api.Name = $Name
    $api.Url = $Url
    $api.Version = $Version
    $api.Method = $Method
    $api.Auth = $Auth
    $api.Headers = $Headers
    $api.Parameters = $Parameters
    $api.ResponseType = $ResponseType
    $api.Timeout = $Timeout
    $api.Enabled = $Enabled
    
    return $api
}

function New-IgnorePattern {
    param(
        [string]$Pattern,
        [bool]$IsRegex = $false,
        [bool]$IsNegated = $false,
        [string]$Category = "",
        [string]$Description = ""
    )
    
    $ignore = [IgnorePattern]::new()
    $ignore.Pattern = $Pattern
    $ignore.IsRegex = $IsRegex
    $ignore.IsNegated = $IsNegated
    $ignore.Category = $Category
    $ignore.Description = $Description
    
    return $ignore
}

# ============================================================================
# Example Usage
# ============================================================================

function Example-Usage {
    Write-Host "╔═══════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║           BMS Download System - Example Usage                    ║" -ForegroundColor Cyan
    Write-Host "╚═══════════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
    
    # Create download manager
    $manager = [DownloadManager]::new()
    
    # Add event handlers
    $manager.OnDownloadStart = {
        param($item)
        Write-Host "[Event] Download started: $($item.FileName)" -ForegroundColor Green
    }
    
    $manager.OnDownloadProgress = {
        param($progress)
        Write-Host "[Event] Progress: $($progress.Progress)%" -ForegroundColor Gray
    }
    
    $manager.OnDownloadComplete = {
        param($item)
        Write-Host "[Event] Download complete: $($item.FileName)" -ForegroundColor Green
    }
    
    $manager.OnDownloadFailed = {
        param($item)
        Write-Host "[Event] Download failed: $($item.FileName) - $($item.Error)" -ForegroundColor Red
    }
    
    $manager.OnAllComplete = {
        Write-Host "[Event] All downloads complete!" -ForegroundColor Green
    }
    
    # Add ignore patterns
    $manager.AddIgnorePattern((New-IgnorePattern -Pattern "*.tmp" -Category "Temp" -Description "Temporary files"))
    $manager.AddIgnorePattern((New-IgnorePattern -Pattern "*.log" -Category "Logs" -Description "Log files"))
    $manager.AddIgnorePattern((New-IgnorePattern -Pattern "*.cache" -Category "Cache" -Description "Cache files"))
    $manager.AddIgnorePattern((New-IgnorePattern -Pattern "node_modules/" -Category "Node" -Description "Node modules"))
    $manager.AddIgnorePattern((New-IgnorePattern -Pattern "__pycache__/" -Category "Python" -Description "Python cache"))
    
    # Add API endpoints
    $manager.AddApiEndpoint((New-ApiEndpoint -Name "Chromium API" -Url "https://www.googleapis.com/download/storage/v1/b/chromium-browser-snapshots/o/Win_x64%2FLAST_CHANGE?alt=media" -Method "GET" -ResponseType "manifest"))
    
    # Add downloads
    $manager.AddDownload((New-DownloadItem -Url "https://www.googleapis.com/download/storage/v1/b/chromium-browser-snapshots/o/Win_x64%2F110.0.5478.0%2Fchrome-win32.zip?alt=media" -Destination "C:\Users\DeLL\Desktop\BMS\build\dependencies\chromium" -FileName "chromium.zip" -Priority [DownloadPriority]::CRITICAL -Type [DownloadType]::BINARY))
    
    $manager.AddDownload((New-DownloadItem -Url "https://storage.googleapis.com/chrome-infra/depot_tools.zip" -Destination "C:\Users\DeLL\Desktop\BMS\build\dependencies" -FileName "depot_tools.zip" -Priority [DownloadPriority]::HIGH -Type [DownloadType]::DEPENDENCY))
    
    # Add packages
    $manager.AddPackage((New-Package -Name "Chromium" -Version "110.0.5478.0" -Source "https://www.googleapis.com/download/storage/v1/b/chromium-browser-snapshots/o/Win_x64%2F110.0.5478.0%2Fchrome-win32.zip?alt=media" -Destination "C:\Users\DeLL\Desktop\BMS\build\dependencies\chromium" -Dependencies @("depot_tools")))
    
    $manager.AddPackage((New-Package -Name "depot_tools" -Version "latest" -Source "https://storage.googleapis.com/chrome-infra/depot_tools.zip" -Destination "C:\Users\DeLL\Desktop\BMS\build\dependencies" -Dependencies @()))
    
    # Add objects
    $manager.AddObject("config", @{
        name = "bms_config"
        version = "1.0.0"
        settings = @{
            debug = $true
            threads = 4
        }
    })
    
    # Start downloads
    $manager.Start()
    
    # Monitor progress
    while ($manager.GetStatistics().IsRunning) {
        Clear-Host
        Write-Host $manager.GetStatus()
        Start-Sleep -Seconds 2
    }
    
    # Export state
    $manager.ExportState("C:\Users\DeLL\Desktop\BMS\download_state.json")
    
    Write-Host "`nAll downloads completed!" -ForegroundColor Green
}

# ============================================================================
# Main Execution
# ============================================================================

if ($MyInvocation.InvocationName -ne '.') {
    Write-Host "╔═══════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║           BMS Download System v1.0                              ║" -ForegroundColor Cyan
    Write-Host "║           PowerShell Download Manager                          ║" -ForegroundColor Cyan
    Write-Host "╚═══════════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
    
    # Run example
    Example-Usage
}