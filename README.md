# Active Directory Control Paths

![An example of control paths graph](graph-example.png "An example of control paths graph")

Control paths in Active Directory are an aggregation of "control relations" between entities of the domain (users, computers, groups, GPO, containers, etc.)
which can be visualized as graphs (such as above) and whose purpose is to answer questions like *"Who can get 'Domain Admins' privileges ?"* or *"What resources a user can control ?"*.

The topic has been presented during a talk at the French conference SSTIC-2014. Our slides and paper can be found here:
[https://www.sstic.org/2014/presentation/chemins\_de\_controle\_active\_directory/](https://www.sstic.org/2014/presentation/chemins_de_controle_active_directory/).

This repository contains tools that can be used to generate such graphs.

---

0. Install / Prerequisites
0. Usage context
0. Dump data into TSV files
0. Import TSV files in graph database
0. Query graph database
0. Visualize graphs
0. Known issues
0. Authors

## 1. INSTALL / PREREQUISITES

### Note:

- **Dump** tools run on Windows (tested on Windows 7 and later).
- **Import**, **Query** and **Visualize** tools should run on anything supporting Neo4j, Java and Ruby, but have mostly been tested on Linux.

### Installation steps (tested on Ubuntu 14.04 and Debian 8):

0. Install Java:

        $ sudo apt-get install openjdk-7-jdk

0. Install Neo4j: download and extract [neo4j community edition](http://www.neo4j.org/download), then create an environment variable with its path. **Do not start the Neo4j server before importing your data.**

        $ tar xzvf neo4j-community-2.3.1-unix.tar.gz
        $ cd neo4j-community-2.3.1

        # the following env variables will be used later
        $ export NEO4J=$PWD # path to neo4j
        $ export CLASSPATH=$NEO4J/lib/*:.

0. Install Ruby:

        $ sudo apt-get install ruby-full

0. Install the `neography` gem:

        $ sudo gem install neography

    If your computer does not have access to the Internet, you can download
    `neography` and all its dependencies using the `bundler` gem. On a separate machine connected
    to the Internet run the following commands:

        $ sudo gem install bundler
        [... installs the bundler gem ...]
        $ cd Query # the Gemfile contains the required gems names, i.e neography
        $ mkdir -p vendor/cache
        $ bundle install --path vendor/cache
        [... downloads neography + dependencies to vendor/cache ...]

    Now copy the `vendor/cache` folder to your offline computer, and run the following
    commands:

        $ cd vendor/cache
        $ sudo gem install neograph-1.7.2.gem
        [... install neography + dependencies ...]

### Tested software versions

- Linux distribution: Ubuntu 14.04 and Debian 8
- Java: 1.7.0
- Neo4j: 2.3.1 (Unix flavor)
- Ruby: 2.5.1
- Neography: 1.8.0

## 2. USAGE CONTEXT

**Note:** No tool needs to run on a domain controller.

Generating control paths graphs for your domain takes the 4 following steps:

0. **Dump** data called **control relations** from the LDAP directory and the SYSVOL.
0. **Import** these relations into a graph-oriented database (Neo4j).
0. **Query** that database to export various nodes lists, control paths, or **create JSON files** representing control paths graphs.
0. **Visualize** SVG graphs created from those JSON files.

![Global process schema of generation of control paths](global-schema.png "Global process schema of generation of control paths")

The 3 last steps are always performed in the same way, but the first step (data dumping) can be carried out in different contexts:

0. Live access to the domain, using a simple domain user account.
0. Live access to the domain, using a domain administrator account.
0. Offline, using a copy of a `ntds.dit` file and a robocopy of the SYSVOL preserving security attributes.

A simple domain user account is enough to dump a large majority of the control relations, but access to a few LDAP containers and GPO folders on the SYSVOL can be denied.
If one is available, an administrator account can thus be used to ensure that no element is inaccessible.

If no access to the domain is given, control graphs can be realized from offline copies of the `ntds.dit` and SYSVOL:

0. A copied `ntds.dit` file can be re-mounted to expose its directory through LDAP with the `dsamain` utility (available on a Windows server machine having the AD-DS or AD-LDS role, or with the "Active Directory Domain Services Tools" installed):

        dsamain.exe -allowNonAdminAccess -dbpath <ntds.dit path>

0. A robocopy of the SYSVOL share preserving security attributes can be done with the `robocopy` utility (the destination folder must be on an NTFS volume):

        robocopy.exe \\<DC ip or host>\sysvol\<domain dns name>\Policies <destination path> /W:1 /R:1 /COPY:DATSO /E /TEE /LOG:<logfile.log>

    **Note**: to preserve security attributes on the copied files you need the `SeRestorePrivilege` on the local computer you're running the robocopy on (that is, you need to run these commands as local administrator).
    You then need to use the `SeBackupPrivilege` to process this local robocopy (dumping tools have a `use backup privilege` option).

## 3. DUMP DATA INTO TSV FILES

**Note:** Dumping tools are available for Windows only (tested on Windows 7 and later).

Use the powershell script `Dump/Dump.ps1` to dump data from the LDAP directory and SYSVOL.
The simplest example is:

    .\Dump.ps1
        -outputDir        <output directory>
        -filesPrefix      <arbitrary prefix>
        -domainController <DC ip or host>
        -sysvolPath       <sysvol 'Policies' folder path>

- `-domainController` can be an real domain controller, or a machine exposing the LDAP directory from a re-mounted `ntds.dit` using `dsamain`.
- `-sysvolPath` can be a network path (example `\\192.168.25.123\sysvol\domain.local\Policies`) or a path to a local robocopy of this folder.

This produces some `.tsv` and `.log` files as follow:

    <outputDir>
        |- logs\*.log           # Log files
        |- dumps\*.tsv          # Unfiltered dumped information
        \- relations\*.tsv      # "Control relations" files, which will be imported in the graph database
        
### Options

- `-help`: show usage.
- `-user` and `-password`: use explicit authentication (by default implicit authentication is used). The username can be specified in the form `DOMAIN\username`, which can be useful to dump a remote domain accessible through a trust.
    If you don't want your password to appear in the command line but still use explicit authentication use the following `runas` command, then launch the `Dump.ps1` script without `-user` and `-password` options:

        C:\> runas /netonly /user:DOM\username cmd.exe
  
- `-logLevel`: change log and output verbosity (possible values are `ALL`, `DBG`, `INFO` (default), `WARN`, `ERR`, `SUCC` and `NONE`).
- `-ldapOnly` and `-sysvolOnly`: dump only data from the LDAP directory (respectively from the SYSVOL).
- `-domainDnsName`: specify the domain dns name (exemple: `domtest.local`), which is not retrieved automatically if you are using a re-mounted `ntds.dit` with `dsamain`.
- `-ldapPort`: change ldap port (default is `389`). This can be useful for a copied `ntds.dit` re-mounted with `dsamain` since it allows you to use a non standard ldap port.
- `-useBackupPriv`: use backup privilege to access `-sysvolPath`, which is needed when using a robocopy. You must use a administrator account to use this option.
- `-generateCmdOnly`: generate the list of commands to use to dump the data, instead of executing these commands. This can be useful on systems where the powershell's execution-policy doesn't allow unsigned scripts to be executed, or on which powershell is not installed in a tested version (v2.0 and later).

## 4. IMPORT TSV FILES IN GRAPH DATABASE

You can now import the dumped "control relations" TSV files in your Neo4j graph database, using the `Import/ControlPathImporter` Java program.

0. Create the following environment variables:

        $ cd neo4j-community-2.3.1
        $ export NEO4J=$PWD # path to neo4j
        $ export CLASSPATH=$NEO4J/lib/*:.

0. Stop the Neo4j server if it is started:

        $ $NEO4J/bin/neo4j stop

0. Compile the import program:

        $ cd Import
        $ make

0. Start the import:

        $ java ControlPathImporter $NEO4J/data/graph.db DUMP/dumps/dom2012r2.obj.ldpdmp.tsv DUMP/relations/*.tsv

0. This will create a Neo4j database. You can then start the Neo4j server:

        $ $NEO4J/bin/neo4j start

0. Setup a password for using Neo4j REST interface. Navigate to
   `http://localhost:7474` (the web interface should be running). Enter the
   default username and password (`neo4j` for both), then enter a new one. The
   querying tool uses `secret` as a default password, but this can be changed
   with the `--password` flag. By default, Neo4j only listens to local
   connections. More information about this can be found on the official Neo4j
   documentation.

### Options

- The first argument is the directory that will contain the created database. The
default for Neo4j is `$NEO4J/data/graph.db`.
- The second argument is the domain object list file, dumped in `<outputDir>/dumps/*.obj.ldpdmp.tsv`.
- The remaining arguments are all the control relations files dumped in `<outputDir>/relations/*.tsv`.

## 5. QUERY GRAPH DATABASE

The `Query/query.rb` program allows you to query the created Neo4j database.

    $ cd ../Query
    $ ./query.rb --help

### Automatic mode

The simplest mode is the "automatic mode", which will create graphs, paths, and nodes lists for a predefined list of builtin targets:

    $ ./query.rb --auto
        [+] running in automatic-mode, lang=en, outdir=out
        [+] control graph for cn=domain admins,cn=users,dc=
        [+] found 13 control nodes, max depth is 5
        [+] control graph with 13 nodes and 12 links
        [+] found 13 control nodes, max depth is 5
        [+] control graph with 13 nodes and 13 links
        [+] found 13 control nodes, max depth is 5
        [...]

The default output directory is `out` and contains the following generated files:

    ./out
       |- *.json         # JSON files containing a graph that can then be visualized as SVG
       |- *_nodes.txt    # Lists of nodes existing in the above graphs
       \- *_paths.txt    # Lists of paths existing in the above graphs

The default target are search with their english DN. You can choose another
language with the `--lang` option. For now, only `en` and `fr` are supported.

### Other examples

The `Query/query.rb` program can also search paths for non-predefined targets, as illustrated in the following examples:

0. Search for nodes (prints node names and their id ; a node's id can be used instead of its DN in every command):

        $ ./query.rb --search 'admins'
            cn=domain admins,cn=users,dc=dom2012r2,dc=local [5]
            cn=enterprise admins,cn=users,dc=dom2012r2,dc=local [6]
            cn=adminsdholder,cn=system,dc=dom2012r2,dc=local [40]
            cn=schema admins,cn=users,dc=dom2012r2,dc=local [166]
            cn=dnsadmins,cn=users,dc=dom2012r2,dc=local [185]

  **Note**: the argument is interpreted as a Java regular expression, enclosed in
`.*` (so that `domain admins` will match the full DN). As a consequence,
certain special characters (such as `{`, found in GPO DN) have a meaning for
the regexp engine. You should manually escape these characters, and search for
`cn=\\{` to match those DN.

0. Get more informations about a node:

        $ ./query.rb --info 'domain admins'
        [...]
        cn=domain admins,cn=users,dc=dom2012r2,dc=local [5]
            type: ["group"]
            directly controlling 245 node(s)
            directly controlled by 6 node(s)


0. Search for nodes controlling another node. We can choose to output results to a
file or to stdout (the `--` stops option processing, so `domain admins` is our
target, and not the output file):

        $ ./query.rb --nodes -- 'domain admins'
            [+] control nodes to node number 5
            [+] found 13 control nodes, max depth is 5
            cn=domain admins,cn=users,dc=dom2012r2,dc=local
            cn=system,cn=wellknown security principals,cn=configuration,dc=dom2012r2,dc=local
            cn=administrators,cn=builtin,dc=dom2012r2,dc=local
            cn=enterprise admins,cn=users,dc=dom2012r2,dc=local
            cn=users,dc=dom2012r2,dc=local
            cn=administrator,cn=users,dc=dom2012r2,dc=local
            cn=builtin,dc=dom2012r2,dc=local
            dc=dom2012r2,dc=local
            cn=domain controllers,cn=users,dc=dom2012r2,dc=local
            cn={31b2f340-016d-11d2-945f-00c04fb984f9},cn=policies,cn=system,dc=dom2012r2,dc=local
            cn=creator owner,cn=wellknown security principals,cn=configuration,dc=dom2012r2,dc=local
            cn=policies,cn=system,dc=dom2012r2,dc=local
            cn=system,dc=dom2012r2,dc=local

0. We can filter the previous result, for example by limiting ourselves to nodes
with the "user" type:

        $ ./query.rb --nodes /tmp/hitlist.txt --nodetype user 'domain admins'
            [+] control nodes with type user to node number 5
            [+] found 13 control nodes, max depth is 5

        $ cat /tmp/hitlist.txt
            cn=administrator,cn=users,dc=dom2012r2,dc=local

0. Search for control paths to our node. We will print all the shortest possible
paths to domain admins into `/tmp/paths.txt`:

        $ ./query.rb --path /tmp/paths.txt --type short 'domain admins'
            [+] short control paths to node number 5
            [+] found 13 control nodes, max depth is 5
            [+] found 38 paths

        $ head -3 /tmp/paths.txt
            cn=domain admins,cn=users,dc=dom2012r2,dc=local
            cn=system,cn=wellknown security principals,cn=configuration,dc=dom2012r2,dc=local stand_right_write_dac cn=domain admins,cn=users,dc=dom2012r2,dc=local
            cn=system,cn=wellknown security principals,cn=configuration,dc=dom2012r2,dc=local stand_right_write_owner cn=domain admins,cn=users,dc=dom2012r2,dc=local

0. Create the full control graph for our node, and save it to
`/tmp/fullgraph.json`:

        $ ./query.rb --graph /tmp/fullgraph.json --type full 'domain admins'
            [+] full control graph to node number 5
            [+] found 13 control nodes, max depth is 5
            [+] control graph with 13 nodes and 59 links

0. By default, we search for paths **to** a node (i.e who can control a node). We
can also try to search for paths **from** a node (i.e what resources are
controlled by this node). Be careful however, as some powerful nodes (such as domain
admins) control everything in the domain. This can lead to never-ending
queries. You can limit the maximum search depth with the `--maxdepth` option.

        $ ./query.rb --path --direction from --type short 'cn=guest,cn=users,dc=dom2012r2,dc=local'
            [+] short control paths from node number 135
            [+] found 2 control nodes, max depth is 1
            [+] found 2 paths
            cn=guest,cn=users,dc=dom2012r2,dc=local
            cn=guests,cn=builtin,dc=dom2012r2,dc=local  group_member cn=guest,cn=users,dc=dom2012r2,dc=local

0. See `./query.rb --help` for a full list of possible options.


## 6. VISUALIZE GRAPHS

Generated JSON files can be visualized using the `Visualize/vizu.html` web page.

0. Copy your JSON files in the `Visualize/data` folder.
0. Run an HTTP server whose document root is the `Visualize` folder:

        $ cd Visualize
        $ python -m SimpleHTTPServer 8000

0. In a browser, generate and visualize the wanted graph with the URL:
        
        http://localhost:8000/vizu.html?json=myproject/mygraph.json

### Options

- `?json=<path>`: relative path in the `Visualize/data` folder of the JSON file to use.
- `&width=<int>&height=<int>`: width and height of the generated graph (by default, dimensions of the window are used).
- `&nolabel`: do not display nodes shortnames.
- `&notooltip`: do not display nodes distinguished names and relations on mouseover.
- `&dist=<int>&charge=<int>`: values used to compute the graph layout (nodes distance and charge, default are respectively `-300` (repulsion between nodes) and `50`).


## 7. KNOWN ISSUES

- Depending on the volume of the Active Directory instance (number of objects and number of relations between them), some tools can be really memory greedy and fail on machines with insufficient RAM. Specifically: dumping tools when LDAP requests return a large amount of data, and querying tools (`neo4j` and `query.rb`) when retrieving large graphs and very deep paths. To mitigate this issue, use machines with a sufficient amount of memory and a 64-bits OS (dumping tools are provided for x64 by default), and use the `--maxdepth` option in your queries.

- On particular datasets, certain queries seems to return paths missing a very few relations, leading to graphs with a small number of "unconnected nodes". The root cause of this issue, which only occurs for some specific datasets, is currently unknown.


## 8. AUTHORS

Lucas Bouillot, Emmanuel Gras - ANSSI - Bureau Audits et Inspections - 2014
