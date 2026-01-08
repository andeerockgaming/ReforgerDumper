[WorkbenchPluginAttribute(name: "Reforger Dumper Manager Plugin", category: "Reforger Dumper", shortcut: "Ctrl+D", wbModules: {"ResourceManager"}, awesomeFontCode: 0xf0c5)]
class ReforgerDumperPluginSettings: ResourceManagerPlugin
{
	// Plugins settings - those can be changed in Plugins -> Settings section 
	[Attribute("1", UIWidgets.CheckBox, "Include acp files", category: "File types to dump")]
	bool m_acp;
	
	[Attribute("1", UIWidgets.CheckBox, "Include ae files", category: "File types to dump")]
	bool m_ae;
	
	[Attribute("1", UIWidgets.CheckBox, "Include agf files", category: "File types to dump")]
	bool m_agf;
	
	[Attribute("1", UIWidgets.CheckBox, "Include agr files", category: "File types to dump")]
	bool m_agr;
	
	[Attribute("1", UIWidgets.CheckBox, "Include anm files", category: "File types to dump")]
	bool m_anm;
	
	[Attribute("1", UIWidgets.CheckBox, "Include asi files", category: "File types to dump")]
	bool m_asi;
	
	[Attribute("1", UIWidgets.CheckBox, "Include ast files", category: "File types to dump")]
	bool m_ast;
	
	[Attribute("1", UIWidgets.CheckBox, "Include asy files", category: "File types to dump")]
	bool m_asy;
	
	[Attribute("1", UIWidgets.CheckBox, "Include aw files", category: "File types to dump")]
	bool m_aw;
	
	[Attribute("1", UIWidgets.CheckBox, "Include bt files", category: "File types to dump")]
	bool m_bt;
	
	[Attribute("1", UIWidgets.CheckBox, "Include c files", category: "File types to dump")]
	bool m_c;
	
	[Attribute("1", UIWidgets.CheckBox, "Include conf files", category: "File types to dump")]
	bool m_conf;
	
	[Attribute("1", UIWidgets.CheckBox, "Include ct files", category: "File types to dump")]
	bool m_ct;
	
	[Attribute("1", UIWidgets.CheckBox, "Include emat files", category: "File types to dump")]
	bool m_emat;
	
	[Attribute("1", UIWidgets.CheckBox, "Include et files", category: "File types to dump")]
	bool m_et;
	
	[Attribute("1", UIWidgets.CheckBox, "Include fnt files", category: "File types to dump")]
	bool m_fnt;
	
	[Attribute("1", UIWidgets.CheckBox, "Include gamemat files", category: "File types to dump")]
	bool m_gamemat;
	
	[Attribute("1", UIWidgets.CheckBox, "Include json files", category: "File types to dump")]
	bool m_json;
	
	/* This crashes the workbench
	[Attribute("1", UIWidgets.CheckBox, "Include layer files", category: "File types to dump")]
	bool m_layer;
	 */
	
	[Attribute("1", UIWidgets.CheckBox, "Include layout files", category: "File types to dump")]
	bool m_layout;
	
	[Attribute("1", UIWidgets.CheckBox, "Include pap files", category: "File types to dump")]
	bool m_pap;
	
	[Attribute("1", UIWidgets.CheckBox, "Include physmat files", category: "File types to dump")]
	bool m_physmat;
	
	[Attribute("1", UIWidgets.CheckBox, "Include ptc files", category: "File types to dump")]
	bool m_ptc;
	
	[Attribute("1", UIWidgets.CheckBox, "Include sig files", category: "File types to dump")]
	bool m_sig;
	
	[Attribute("1", UIWidgets.CheckBox, "Include siga files", category: "File types to dump")]
	bool m_siga;
	
	[Attribute("1", UIWidgets.CheckBox, "Include styles files", category: "File types to dump")]
	bool m_styles;
	
	[Attribute("1", UIWidgets.CheckBox, "Include terr files", category: "File types to dump")]
	bool m_terr;
	
	[Attribute("1", UIWidgets.CheckBox, "Include vhcsurf files", category: "File types to dump")]
	bool m_vhcsurf;
	
	/* This crashes the workbench
	[Attribute("1", UIWidgets.CheckBox, "Include xob files", category: "File types to dump")]
	bool m_xob;
	 */
	
	[Attribute("1", UIWidgets.CheckBox, "Ignore files in the Workbench Game directory", category: "Settings")]
	bool m_IgnoreWorkbenchGame;
	
	static ReforgerDumperPluginSettings s_Instance;
	static ref array<string> s_FilesToDump;
	static ref set<string> s_CreatedDirs;
	static int s_QueueIndex;
	
	[ButtonAttribute("Dump")]
	void DumpFiles()	
	{
		s_Instance = this;
		s_FilesToDump = new array<string>();
		s_CreatedDirs = new set<string>();
		s_QueueIndex = 0;
		
		SearchResourcesFilter filter = new SearchResourcesFilter();
		filter.fileExtensions = {};
		GetEnabledFileExtensions(filter.fileExtensions);

		Print("Reforger Dumper: Scanning for resources... (UI may freeze briefly)");
		ResourceDatabase.SearchResources(filter, SearchResourcesCallback);
		
		int total = s_FilesToDump.Count();
		Print("Reforger Dumper: Should dump " + total + " files.");
		
		if (total > 0)
		{
			if (GetGame() && GetGame().GetCallqueue())
			{
				Print("Reforger Dumper: Starting async dump (Check logs for progress)...");
				GetGame().GetCallqueue().CallLater(ProcessQueue, 0, true);
			}
			else
			{
				Print("Reforger Dumper: WARNING - Game CallQueue unavailable. Running synchronously (App will freeze!)");
				ProcessQueueSync();
			}
		}
	}
	
	// Callback run when a resource is found - now just collects paths
	static void SearchResourcesCallback(ResourceName resName, string filePath = "")
	{
		if (filePath == string.Empty)
		{
			// Try to fallback
			if (resName) 
				filePath = resName.GetPath();
				
			if (filePath == string.Empty) return;
		}

		// Quick filter check
		if (s_Instance && s_Instance.m_IgnoreWorkbenchGame && filePath.Contains("WorkbenchGame"))
		{
			return;
		}
		
		s_FilesToDump.Insert(filePath);
	}
	
	static void ProcessQueue()
	{
		// Process X files per frame
		int batchSize = 50; 
		int total = s_FilesToDump.Count();
		
		for (int i = 0; i < batchSize; i++)
		{
			if (s_QueueIndex >= total)
			{
				GetGame().GetCallqueue().Remove(ProcessQueue);
				Print("Reforger Dumper: Dump Finished! " + total + " files processed.");
				Print("Please move the files out of the profile directory for the sake of Workbench performance and then restart it.");
				return;
			}
			
			string file = s_FilesToDump[s_QueueIndex];
			ProcessFile(file);
			s_QueueIndex++;
		}
		
		if (s_QueueIndex % 1000 < batchSize)
		{
			Print("Reforger Dumper: Progress " + s_QueueIndex + " / " + total);
		}
	}
	
	static void ProcessQueueSync()
	{
		int total = s_FilesToDump.Count();
		for (int i = 0; i < total; i++)
		{
			ProcessFile(s_FilesToDump[i]);
			if (i % 1000 == 0) Print("Reforger Dumper: Progress " + i + " / " + total);
		}
		Print("Reforger Dumper: Dump Finished!");
	}
	
	static void ProcessFile(string filePath)
	{
		int colonIndex = filePath.IndexOf(":");
		if (colonIndex == -1) return;
		
		string rootDir = filePath.Substring(1, colonIndex - 1);
		if (rootDir.Contains("profile") || rootDir.Contains("Profile")) return;
		
		string localPath = "Dump/" + rootDir + "/" + filePath.Substring(colonIndex + 1, filePath.Length() - colonIndex - 1);
		
		// Directory creation with caching
		// Get directory path from localPath
		int lastSlash = localPath.LastIndexOf("/");
		if (lastSlash != -1)
		{
			string dirPath = "$profile:" + localPath.Substring(0, lastSlash);
			if (!s_CreatedDirs.Contains(dirPath))
			{
				// We still need to ensure parent structure exists, but Enfusion FileIO MakeDirectory might not be recursive?
				// Original code did recursive split.
				// Let's do a simplified recursive check but cache results.
				
				// Re-implementing simplified recursive create with cache
				array<string> tempFolders = new array<string>();
				localPath.Split("/", tempFolders, true);
				int count = tempFolders.Count() - 1; // last is filename
				string currentFolderPath = "$profile:";
				
				for (int i = 0; i < count; i++)
				{
					currentFolderPath += tempFolders[i];
					if (!s_CreatedDirs.Contains(currentFolderPath))
					{
						if (!FileIO.FileExists(currentFolderPath))
						{
							FileIO.MakeDirectory(currentFolderPath);
						}
						s_CreatedDirs.Insert(currentFolderPath);
					}
					currentFolderPath += "/";
				}
			}
		}

		WriteFile(filePath, "$profile:" + localPath);
	}
	
	// Reads from the ResourceName and writes to the local file system
	static void WriteFile(string inputPath, string outputPath)
	{
		// Use optimized CopyFile instead of manual read/write loop to support binary files and speed up processing
		// Logic handles prefixes
		FileIO.CopyFile(inputPath, outputPath); 
	}
	
	// Ugly way to insert the enabled extensions into the array
	void GetEnabledFileExtensions(out notnull array<string> extensions)
	{
		if (m_acp)
			extensions.Insert("acp");
		if (m_ae)
			extensions.Insert("ae");
		if (m_agf)
			extensions.Insert("agf");
		if (m_agr)
			extensions.Insert("agr");
		if (m_anm)
			extensions.Insert("anm");
		if (m_asi)
			extensions.Insert("asi");
		if (m_ast)
			extensions.Insert("ast");
		if (m_asy)
			extensions.Insert("asy");
		if (m_aw)
			extensions.Insert("aw");
		if (m_bt)
			extensions.Insert("bt");
		if (m_c)
			extensions.Insert("c");
		if (m_conf)
			extensions.Insert("conf");
		if (m_ct)
			extensions.Insert("ct");
		if (m_emat)
			extensions.Insert("emat");
		if (m_et)
			extensions.Insert("et");
		if (m_fnt)
			extensions.Insert("fnt");
		if (m_gamemat)
			extensions.Insert("gamemat");
		if (m_json)
			extensions.Insert("json");
		/* This crashes the workbench
		if (m_layer)
			extensions.Insert("layer");
		 */
		if (m_layout)
			extensions.Insert("layout");
		if (m_pap)
			extensions.Insert("pap");
		if (m_physmat)
			extensions.Insert("physmat");
		if (m_ptc)
			extensions.Insert("ptc");
		if (m_sig)
			extensions.Insert("sig");
		if (m_siga)
			extensions.Insert("siga");
		if (m_styles)
			extensions.Insert("styles");
		if (m_terr)
			extensions.Insert("terr");
		if (m_vhcsurf)
			extensions.Insert("vhcsurf");
		/* This crashes the workbench
		if (m_xob)
			extensions.Insert("xob");
		 */
	}
	
	override void Configure()
	{
		Workbench.ScriptDialog("Configure settings", "", this);
	}
	
	// I don't know if I need this
	override void Run()
	{
		Workbench.ScriptDialog("Configure settings", "", this);
		super.Run();
	}
}