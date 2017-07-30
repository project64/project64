using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using System.Deployment.Application;

namespace Project64.ClickOnce
{
	class Launcher
	{
		public static void Main()
		{
			try
			{
				Environment.SpecialFolder folder = Environment.SpecialFolder.LocalApplicationData;
				string localPath = Environment.GetFolderPath(folder);
				string localAppPath = Path.Combine(localPath, "Hyvart Software", "Project64Dev");
				string configPath = Path.Combine(localAppPath, "Config");

				// If local config folder exists, offer to keep/overwrite.
				if (ApplicationDeployment.CurrentDeployment.IsFirstRun && Directory.Exists(configPath))
				{
					string message = string.Format("Current configuration at {0} exists.\nDo you wish to overwrite?\nWARNING: You will lose your current settigs.", configPath);
					DialogResult result = MessageBox.Show(message, "", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button2);

					if (result == DialogResult.Yes)
						DeployConfigLocally(localAppPath);
				}
				else if (!Directory.Exists(configPath))
				{
					Directory.CreateDirectory(configPath);
					DeployConfigLocally(localAppPath);
				}

				string cfgPath = Path.Combine(Path.Combine(localAppPath, "Config"), "Project64.cfg");
				File.WriteAllText(Path.Combine("Config", "Project64.cfg"), string.Format("[default]\r\nConfigFile={0}\r\n", cfgPath));
				Process.Start("Project64.exe");
			}
			catch (Exception ex)
			{
				MessageBox.Show(string.Format("An error has been found.\n\n{0}", ex));
			}
		}

		/// Create local (version-independent) config path.
		static void DeployConfigLocally(string localAppPath)
		{
			File.Copy(Path.Combine("Config", "Project64.cfg"), Path.Combine(localAppPath, "Config", "Project64.cfg"), true);
		}
	}
}
