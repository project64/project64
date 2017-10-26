Param
(
	$SolutionDir = (Split-path (Split-Path ($PSScriptRoot))),
	[Parameter(Mandatory=$true)]
	[string] $Version,
	[string[]] $Files = (
		"$SolutionDir\Source\Project64-core\version.h",
		"$SolutionDir\Source\nragev20\version.h",
		"$SolutionDir\Source\RSP\version.h",
		"$SolutionDir\Source\Project64-video\version.h"
	)
)

$versionArr = $Version.Split('.')
foreach ($file in $Files) {
	(Get-Content $file) `
		-replace '(#define\s+VERSION_MAJOR\s+)\d+',    "`${1}$($versionArr[0])" `
		-replace '(#define\s+VERSION_MINOR\s+)\d+',    "`${1}$($versionArr[1])" `
		-replace '(#define\s+VERSION_REVISION\s+)\d+', "`${1}$($versionArr[2])" `
		-replace '(#define\s+VERSION_BUILD\s+)\d+',    "`${1}$($versionArr[3])" `
	| Set-Content $file
}
