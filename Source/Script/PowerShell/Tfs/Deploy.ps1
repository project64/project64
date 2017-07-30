<# Deploy generated artifacts from current directory to target installation directory. #>
If ($args.Count -lt 1)
{
    Write-Host "Missing target param. Paila."
    Exit 1
}

# Validate input: 1 param: target directory. Must exist.
$source = $(Get-Location).Path
$target = $args[0]
$config = "Release"
# If there is a 2nd arg, assume it is the configuration to copy.
if ($args.Count -gt 1)
{
    Write-Host "Overwriting `$config as $($args[1])"
    $config = $args[1]
}
Write-Host $config

# Copy Config files
# Appending '$target\Config' in case $target desn't exist yet.
Copy-Item -Recurse -Path $source\Config -Destination $target\Config
Copy-Item -Path $PSScriptRoot\Project64.cfg -Destination $target\Config\
Remove-Item -Path $target\Config\Project64.cfg.development

# Copy Plugins
Robocopy.exe $source\Plugin $target\Plugin\ *.dll /s

# Copy Lang files
Copy-Item -Recurse -Path $source\Lang -Destination $target\

# Copy executable(s)
Copy-Item -Path $source\Bin\$config\Project64.exe -Destination $target\