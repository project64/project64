Param
(
	$SolutionDir,
	$ArtifactDir,
	$TargetDir,
	$Version,
	$ProviderUrlBase,
	$AppName = 'Project64',
	$Platform = 'Win32',
	# 4.0 required for Windows XP support.
	$Mage = "${env:ProgramFiles(x86)}\Microsoft SDKs\Windows\v8.0A\bin\NETFX 4.0 Tools\mage.exe",
	$CertFile = '',
	$CertPass = ''
)
<# 
 -SolutionDir $(Build.SourcesDirectory)
 -ArtifactDir BUILD_ARTIFACTSTAGINGDIRECTORY
 -Version $(ClickOnceVersion)
 -ProviderUrlBase $(ProviderUrlBase)
 -AppName $(AppName)
 -Platform $(BuildPlatform)
 -CertFile $(CertFile)
 -CertPass $(CertPass)
#>

$processors = @{
	'Win32' = 'x86';
	'x64' = 'x86'	# For now, we're sticking to an x86 launcher.
}

If (! $TargetDir) # If ArtifactDir is provided but no TargetDir, make them the same.
{
	$TargetDir = "$ArtifactDir\$Version-$Platform" #Tfs-specific naming convention.
}

$appManifest = "$AppName.manifest"
$deployManifest = "$AppName.application"

# CRITICAL: Copy Launcher.exe to $TargetDir for it to show up in the manifest. It will not be included in the published drop.
# Check if already exists (SolutionDir = TargetDir)
If (! $(Test-Path $TargetDir\Launcher.exe) )
{
	Copy-Item $SolutionDir\Launcher.exe $TargetDir\
}

# Create application manifest
& $Mage -New Application -Processor $processors[$Platform] -Name "$AppName" -Version $Version `
	-FromDirectory $TargetDir -ToFile $SolutionDir\$appManifest

# Sign application manifest BEFORE generating deployment manifest from it.
If ($CertFile -and $CertPass)
{
	& $Mage -Sign $SolutionDir\$appManifest -CertFile $CertFile -Password $CertPass
}

#TODO: don't force /clickonce/ as part as the URL base.
# Create deployment manifest
& $Mage -New Deployment -Processor $processors[$Platform] -Publisher "Hyvart Software" -Version $Version -Install true `
	-ProviderUrl $ProviderUrlBase/clickonce/$deployManifest -AppCodeBase $ProviderUrlBase/clickonce/$appManifest `
	-AppManifest $SolutionDir\$appManifest -ToFile $SolutionDir\$deployManifest

# Sign deployment manifest, if applicable.
If ($CertFile -and $CertPass)
{
	& $Mage -Sign $SolutionDir\$deployManifest -CertFile $CertFile -Password $CertPass
}