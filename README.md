# Active Directory Control Paths
## "Who Can Read the CEO's Emails Edition"
![An example of control paths graph](graph-example.png "An example of control paths graph")

Control paths in Active Directory are an aggregation of "control relations" between entities of the domain (users, computers, groups, GPO, containers, etc.). 
These relations can be visualized as graphs (such as above) to answer questions like *"Who can get 'Domain Admins' privileges ?"* or *"What resources can a user control ?"* and even *"Who can read the CEO's emails ?"*.

---
## CHANGES
Basic Cypher querying through Neo4j REST API, increasing performance

New control paths are added : Kerberos delegation, SCCM dumping utilities for local admins and sessions control paths

Adding EXCHANGE permissions in v1.3 "Who Can Read the CEO's Emails Edition".
Permissions extracted from AD Users, Mailbox/DB descriptors, RBAC and MAPI folders.

Better resume features, nodes clustering (through OVALI) in v1.2.3.

New control paths are added in v1.2.2: RoDC and LAPS.

Major code changes take place in v1.2, as it is now able to dump and analyze very large Active Directories without hogging too much RAM.
Some very large ADs with over 1M objects and 150M ACEs have been processed in a reasonable amount of time (a few hours on a laptop, consuming less than 1GB RAM).


---
## QUICK START
- Download and extract the latest binary release from the Github Releases tab on a Windows machine with Java.
- Skip to part 3 (Dump step)


---
## TABLE OF CONTENT
1. Install / Prerequisites
2. Usage context
3. Dump data into CSV files
4. Import CSV files in graph database
5. Query graph database
6. Visualize graphs
7. Authors


## 1. INSTALL / PREREQUISITES

### Note

- **Dump** step runs on Windows only (tested on Windows 7 and later).
- **Import**, **Query** and **Visualize** steps can run on the same machine or on anything supporting Neo4j, Java and Ruby. They have been tested on Windows and Linux.

### Building

 - Build the 3 solutions in the subfolders of /Dump/Src/ with an up-to-date Visual Studio (Community version works). Targets must be:
   - Release/x64 for AceFilter
   - Release/x64 for ControlRelationProviders
   - RelADCP/x64 for DirectoryCrawler.

### Prerequisites

0. Install a Java JRE from https://java.com/en/download/manual.jsp or from your distribution.

0. Install Neo4j: download the latest build of [neo4j community edition](https://neo4j.com/download/other-releases/) and extract the zip/tar archive (not the installer). **Do not start the Neo4j server before importing your data.**

0. Install Neo4j service as admin: .\bin\neo4j.bat install-service -Verbose

0. Modify Neo4j default configuration file: conf/neo4j.conf. Set the following:
```	
    dbms.active_database=adcp.db
    cypher.forbid_shortestpath_common_nodes=false
```
0. Start the Neo4j server:
```
    .\bin\neo4j start -Verbose
```

0. Change the default password for the Neo4j REST interface. Navigate to `http://localhost:7474` (the web interface should be running). 
   Enter the default username and password (`neo4j` for both), then enter `secret` as your new password (Default in the Query script, can be changed with -neo4jPassword).
   By default, Neo4j only listens to local connections.



## 2. USAGE CONTEXT

**Note:** None of these tools need to run on a domain controller.

Generating control paths graphs for your domain takes the 4 following steps:

0. **Dump** data from LDAP directory, SYSVOL, SCCM and EWS and run analyzers to form control relationships.
0. **Import** these relations into a graph-oriented database (Neo4j).
0. **Query** that database to export various nodes lists, control paths, or **create JSON files** representing control paths graphs.
0. **Visualize** graphs created from those JSON files.

![Global process schema of generation of control paths](global-schema.png "Global process schema of generation of control paths")

The 3 last steps are always performed in the same way, but the first step (data dumping) can be carried out in different contexts:

0. Live access to the domain, using a simple domain user account.
0. Live access to the domain, using a domain administrator account.
0. Offline, using a copy of a `ntds.dit` file and a robocopy of the SYSVOL preserving security attributes.

A simple domain user account is enough to dump a large majority of the control relations, but access to a few LDAP containers and GPO folders on the SYSVOL can be denied.
If one is available, an administrator account can thus be used to ensure that no element is inaccessible.

If no access to the domain is given, control graphs can be realized from offline copies of the `ntds.dit` and SYSVOL:

0. A copied `ntds.dit` file can be re-mounted to expose its directory through LDAP with the `dsamain` utility (available on a Windows server machine having the AD-DS or AD-LDS role, or with the "Active Directory Domain Services Tools" installed):

        dsamain.exe -allowNonAdminAccess -dbpath <ntds.dit path> -ldapPort 1234

0. A robocopy of the SYSVOL share preserving security attributes can be done with the `robocopy` utility (the destination folder must be on an NTFS volume):

        robocopy.exe \\<DC ip or host>\sysvol\<domain dns name>\Policies <destination path> /W:1 /R:1 /COPY:DATSO /E /TEE /LOG:<logfile.log>

    **Note**: to preserve security attributes on the copied files you need the `SeRestorePrivilege` on the local computer you're running the robocopy on (that is, you need to run these commands as local administrator).
    You then need to use the `SeBackupPrivilege` to process this local robocopy (dumping tools have a `use backup privilege` option that you must use).

## 3. DUMP DATA INTO CSV FILES


Use the powershell script `Dump\Dump.ps1` to dump data from the LDAP directory and SYSVOL.
The simplest example is:

    .\Dump.ps1
      -outputDir        <output directory>
      -domainController <DC ip or host>
      -domainDnsName    <domain FQDN>

- `-domainController` can be an real domain controller, or a machine exposing the LDAP directory from a re-mounted `ntds.dit` using `dsamain`.


This produces some `.csv` and `.log` files as follow:

    <outputDir>
        |- yyyymmdd_domainfqdn\Logs\*.log                                    # Log files
        |- yyyymmdd_domainfqdn\Ldap\*.csv                                    # Unfiltered dumped information
        \- yyyymmdd_domainfqdn\Relations\*.csv                               # "Control relations" files, which will be imported into the graph database
        
### Other options

- `-help`: show usage.
- `-user` and `-password`: use explicit authentication (by default implicit authentication is used). The username can be specified in the form `DOMAIN\username`, which can be useful to dump a remote domain accessible through a trust.
    If you don't want your password to appear in the command line but still use explicit authentication use the following `runas` command, then launch the `Dump.ps1` script without `-user` and `-password` options:

        C:\> runas /netonly /user:DOM\username powershell.exe
- `-sysvolPath` can be a network path (example `\\192.168.25.123\sysvol\domain.local\Policies`) or a path to a local robocopy of this folder. Defaults to `\\domainController\sysvol\domainDnsName\Policies`.
- `-exchangeServer`, `-exchangeUser` and `-exchangePassword`: explicit authentication for EWS on a CAS Exchange server. 
  Use an Exchange Trusted Subsystem member account with an active mailbox, but NOT DA/EA/Org Mgmgt because of some Deny ACEs.
  
- `-logLevel`: change log and output verbosity (possible values are `ALL`, `DBG`, `INFO` (default), `WARN`, `ERR`, `SUCC` and `NONE`).
- `-ldapOnly` and `-sysvolOnly`: dump only data from the LDAP directory (respectively from the SYSVOL).
- `-ldapPort`: change ldap port (default is `389`). This can be useful for a copied `ntds.dit` re-mounted with `dsamain` since it allows you to use a non standard ldap port.
- `-useBackupPriv`: use backup privilege to access `-sysvolPath`, which is needed when using a robocopy. You must use an administrator account to use this option.
- `-generateCmdOnly`: generate the list of commands to use to dump the data, instead of executing these commands. This can be useful on systems where the powershell's execution-policy doesn't allow unsigned scripts to be executed, or on which powershell is not installed in a tested version (v2.0 and later).
- `-fromExistingDumps`: skip the LDAP request step and work from files found in the Ldap\ folder.
- `-resume`: look for the first non-successful command in the same-day, same-target folder and resume from there. Can be used to resume if your connection to the DC went down.
- `-forceOverwrite`: overwrite any previous dump files from the same-day, same-target folder

**Warning:** Accessing the Sysvol share from a non-domain machine can be blocked by UNC Paths hardening, which is a client-side parameter enabled by default since Windows 10. Disable it like this:
Set-ItemProperty -Path HKLM:\Software\Policies\Microsoft\Windows\NetworkProvider\HardenedPaths -Name "\\*\SYSVOL" -Value "RequireMutualAuthentication=0"


## 4. IMPORT CSV FILES INTO A GRAPH DATABASE

You can now import the Relations CSV files along with the AD objects into your Neo4j graph database. This step can be done fully offline.
You may need admin permissions to start/stop Neo4j.


0. Stop the Neo4j server if it is started:
```
    .\bin\neo4j stop
```
0. Import CSV files in a new graph database adcp.db:

- Set an environment variable to the dump folder for convenience:
```
    $env:DUMP = "PATH_TO\yyyymmdd_domainfqdn\" 
```
- In neo4j folder (you can copy/paste this):
```
    .\bin\neo4j-import --into data/databases/adcp.db --id-type string  `
    --nodes $env:DUMP\Ldap\all_nodes.csv  `
    --relationships $((dir $env:DUMP\relations\*.csv -exclude *.deny.csv) -join ',') `
    --input-encoding UTF-16LE --multiline-fields=true --legacy-style-quoting=false
```

  Headers-related errors will be raised and can be ignored. It is still a good idea to have a look at the bad.log file.
  Do not use the "admin-tool import" command, even though it is supposed to do the same thing. Parameters passing is currently bugged.
		
0. Restart the Neo4j server if it is stopped:
```
    .\bin\neo4j start
    

## 5. QUERYING THE GRAPH DATABASE

The `Query/Query.ps1` script is used to query the created Neo4j database.
	
### Basic query to get a graph and paths of all nodes able to take control of the "Domain Admins" group:

    .\Query.ps1 -quick
	  
   
### To search for a node from its DN or an email address and get a graph to it (useful if AD is not in English):

    .\Query.ps1 -search "cn=administrateurs,"
    
    .\Query.ps1 -search "ceo@domain.local"
    
  (This will return a node id number)
  
    .\Query.ps1 -graph <node id number> -outFile <JSON filename>
    
  (This produces a json graph file, which you can visualize, see part 6)

  
### Progressively increase the ShortestPath algorithm Depth parameter as you visualize and adjust the graph

    .\Query.ps1 -graph <node id number> -maxDepth 15 -outFile <JSON filename>
    
    
    
## 6. VISUALIZE GRAPHS

ADCP uses the [OVALI](https://github.com/ANSSI-FR/OVALI) frontend to display
JSON data files as graphs.

0. Quick Start
Open Visualize/index.html with a web brower (Chrome/Chromium is preferred).
Open one of the generated json files.

For better visibility, you might want to:
- right click -> cluster some similar nodes
- setup hierarchical viewing with the menu on the left, especially for email nodes as this will flatten Exchange RBAC nodes
- disable physics if the graph does not stabilize
- remove unwanted relationships or nodes with right click -> "Cypher delete to clipboard" and paste into http://localhost:7474 then relaunch the query.




## 7. AUTHORS

Geraud de Drouas - 2018
Geraud de Drouas - ANSSI - 2015-2017

Lucas Bouillot, Emmanuel Gras - ANSSI - 2014
Presented at the French conference SSTIC-2014. Slides and paper can be found here:
[https://www.sstic.org/2014/presentation/chemins\_de\_controle\_active\_directory/](https://www.sstic.org/2014/presentation/chemins_de_controle_active_directory/).
