param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [switch]$Check
)
$ErrorActionPreference = 'Stop'

# Check the explicit Visual Studio inventory before building, so new SDK/feature files cannot be silently omitted.
[xml]$project = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'ext_client.vcxproj')
$namespace = New-Object System.Xml.XmlNamespaceManager($project.NameTable)
$namespace.AddNamespace('msb', $project.DocumentElement.NamespaceURI)
$items = @($project.SelectNodes('//msb:ClCompile[@Include]', $namespace) | ForEach-Object { $_.Include })
$listed = @($items | Where-Object { $_ -like '20-Program\*' } | ForEach-Object { $_.ToLowerInvariant() })
$root = Join-Path $PSScriptRoot '20-Program'
$actual = @(Get-ChildItem -LiteralPath $root -Recurse -File -Filter '*.cpp' | ForEach-Object {
    $_.FullName.Substring($PSScriptRoot.Length + 1).ToLowerInvariant()
})
$difference = @(Compare-Object $actual $listed)
if ($difference.Count) { throw "C++ source inventory differs from ext_client.vcxproj: $($difference.InputObject -join ', ')" }
if (@($items | Group-Object | Where-Object Count -gt 1).Count) { throw 'Duplicate C++ project items' }
foreach ($item in $items) {
    if (!(Test-Path -LiteralPath (Join-Path $PSScriptRoot $item))) { throw "Missing project source: $item" }
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio with Desktop development with C++.' }
$msbuild = & $vswhere -latest -prerelease -products '*' -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (!$msbuild) { throw 'MSBuild with the Visual C++ Win32 toolchain was not found.' }

function Invoke-NativeBuild([string]$ProjectPath, [string]$BuildConfiguration) {
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $msbuild
    $info.Arguments = '"' + $ProjectPath + '" /p:Configuration=' + $BuildConfiguration + ' /p:Platform=Win32 /m /nologo /v:minimal'
    $info.WorkingDirectory = $PSScriptRoot
    $info.UseShellExecute = $false
    # Rebuild the case-insensitive Windows environment to remove duplicate Path/PATH keys inherited from launchers.
    $info.EnvironmentVariables.Clear()
    foreach ($entry in [Environment]::GetEnvironmentVariables().GetEnumerator()) {
        $info.EnvironmentVariables[$entry.Key] = $entry.Value
    }
    $process = [System.Diagnostics.Process]::Start($info)
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "MSBuild failed with exit code $($process.ExitCode)" }
}

Invoke-NativeBuild (Join-Path $PSScriptRoot 'sro_ext_client.sln') $Configuration
if ($Check) {
    Invoke-NativeBuild (Join-Path $PSScriptRoot 'tests\client_checks.vcxproj') 'Release'
    & (Join-Path $PSScriptRoot '..\Output\Checks\client_checks.exe')
    if ($LASTEXITCODE -ne 0) { throw "Client checks failed with exit code $LASTEXITCODE" }
}
Write-Host "Output: $(Join-Path $PSScriptRoot "..\Output\$Configuration\ext_client.dll")"

# Automatic deployment to game client directories if present
$builtDll = Join-Path $PSScriptRoot "..\Output\$Configuration\ext_client.dll"
$deployTargets = @(
    'Z:\E\Workspace\Silkroad\RIGID_v234',
    'C:\Users\alpka\Desktop\Game\debug'
)
foreach ($targetDir in $deployTargets) {
    if (Test-Path -LiteralPath $targetDir) {
        $destPath = Join-Path $targetDir 'ext_client.dll'
        try {
            Copy-Item -LiteralPath $builtDll -Destination $destPath -Force
            Write-Host "Deployed: $destPath"
        } catch {
            # If in-use, move existing to .old and copy new DLL
            try {
                $tempOld = Join-Path $targetDir "ext_client.old.dll"
                if (Test-Path -LiteralPath $tempOld) { Remove-Item -LiteralPath $tempOld -Force -ErrorAction SilentlyContinue }
                Move-Item -LiteralPath $destPath -Destination $tempOld -Force
                Copy-Item -LiteralPath $builtDll -Destination $destPath -Force
                Write-Host "Deployed (replaced in-use): $destPath"
            } catch {
                Write-Warning "Could not deploy to $destPath (file may be in use by running game client): $_"
            }
        }
    }
}

