Param
(
    $SolutionDir = $(Get-Location),
    $Config = 'Release',
    $Platform = 'Win32',
    $ProviderUrlBase
)
<#
    -u "$(VsoScript.User):$(VsoScript.Password)"
    -O "$(VsoScript.RepoUrl)?scopepath=/$(VsoScript.Arrange)"
    -O "$(VsoScript.RepoUrl)?scopepath=/$(VsoScript.ClickOnce)"
    -O "$(VsoScript.RepoUrl)?scopepath=/$(VsoScript.Launcher)"
    -O "$(VsoScript.RepoUrl)?scopepath=/$(VsoScript.Cfg)"
#>

$suffixes = @{
    'Win32' = '';
    'x64' = '64'
};

$binFolder = $Config + $suffixes[$Platform];

# Move Project64.exe into root directory
Move-Item $SolutionDir\Bin\$binFolder\Project64.exe $SolutionDir\

# Move launcher into root directory
#Move-Item $SolutionDir\Source\ClickOnce\bin\$Config\Launcher.exe $SolutionDir\

# Delete development config template
Remove-Item $SolutionDir\Config\Project64.cfg.development

# Copy config template
Copy-Item $SolutionDir\Project64.cfg $SolutionDir\Config\

# x64 specifics: rename Plugin64 as Plugin, download audio plugin.
IF ($Platform -eq 'x64')
{
    Remove-Item -Force -Recurse $SolutionDir\Plugin # So Win32 Jabo's plugins don't get deployed
    mkdir $SolutionDir\Plugin64\Audio
    
    # Download audio plugin so stuff actually works.
    Invoke-WebRequest $ProviderUrlBase/plugins/$Platform/no_sound.dll -OutFile $SolutionDir\Plugin64\Audio\no_sound.dll
}