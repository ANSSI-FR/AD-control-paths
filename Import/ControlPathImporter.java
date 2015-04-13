import java.io.*;
import java.util.*;

import org.neo4j.graphdb.*;
import org.neo4j.unsafe.batchinsert.*;

public class ControlPathImporter {
  private BatchInserter inserter;
  private Map<String, Long> nodesId;
  private Map<String, String> nodesType;
  private long importedNodes = 0;
  private long importedRels = 0;

  public ControlPathImporter(String storeDir) {
    inserter = BatchInserters.inserter(storeDir);
    nodesId = new HashMap<String, Long>();
    nodesType = new HashMap<String, String>();
  }

  void loadMapping(String mappingPath) throws IOException {
    File mappingFile = new File(mappingPath);
    FileReader fileReader = new FileReader(mappingFile);
    BufferedReader bufferedReader = new BufferedReader(fileReader, 32*1024*1024);
    String line;
    String tokens[];
    String toktokens[];

    line = bufferedReader.readLine(); // skip first line (header)

    while ((line = bufferedReader.readLine()) != null) {
      tokens = line.toLowerCase().split("\t");

      // we need at least 2 columns
      if (tokens.length < 2) {
        continue;
      }
      // type is the last element of the comma-separated second column
      toktokens = tokens[1].split(",");
      nodesType.put(tokens[0], toktokens[toktokens.length-1]);
    }

    System.out.println(" " + nodesType.size() + " objects in mapping");
  }

  void doImport(String dataPath) throws IOException {
    long skippedLines = 0;
    long nodeTotal = 0;
    long relTotal = 0;
    File dataFile = new File(dataPath);
    FileReader fileReader = new FileReader(dataFile);
    BufferedReader bufferedReader = new BufferedReader(fileReader, 32*1024*1024);
    Map<String, Object> properties = new HashMap<>();
    String line;
    String tokens[];
    long from;
    long to;
    RelationshipType rel;
    Label label;

    line = bufferedReader.readLine(); // skip first line (header)

    while ((line = bufferedReader.readLine()) != null) {
      tokens = line.toLowerCase().split("\t");

      if (tokens.length != 3) {
        skippedLines++;
        continue;
      }

      // if "from" the node already exists, use it, else create it
      if (nodesId.containsKey(tokens[0])) {
        from = nodesId.get(tokens[0]);
      } else {
        properties.put("name", tokens[0]);

        // set the node label using the mapping file if we know the node type
        if (nodesType.containsKey(tokens[0])) {
          label = DynamicLabel.label(nodesType.get(tokens[0]));
          from = inserter.createNode(properties, label);
        } else {
          from = inserter.createNode(properties);
        }
        nodeTotal++;
        nodesId.put(tokens[0], from);
      }

      // if "to" the node already exists, use it, else create it
      if (nodesId.containsKey(tokens[1])) {
        to = nodesId.get(tokens[1]);
      }
      else {
        properties.put("name", tokens[1]);

        // set the node label using the mapping file if we know the node type
        if (nodesType.containsKey(tokens[1])) {
          label = DynamicLabel.label(nodesType.get(tokens[1]));
          to = inserter.createNode(properties, label);
        } else {
          to = inserter.createNode(properties);
        }
        nodeTotal++;
        nodesId.put(tokens[1], to);
      }

      // create relationship
      rel = DynamicRelationshipType.withName(tokens[2]);
      inserter.createRelationship(from, to, rel, null);
      relTotal++;
    }

    importedNodes += nodeTotal;
    importedRels += relTotal;
    System.out.println(" " + nodeTotal + " new node(s), " + relTotal + " new relationship(s), skipped " + skippedLines + " line(s)");
  }

  void shutdown() {
    inserter.shutdown();
  }

  public static void main(String[] args) {
    int i = 0;

    if (args.length < 3) {
      System.err.println("Usage:\tjava ControlPathImporter [GRAPH-DB] [OBJECT-LIST] [RELATIONS]");
      System.err.println("\tGRAPH-DB   : path to the graph database folder (usually <neo4j>/data/graph.db/)");
      System.err.println("\tOBJECT-LIST: path to the domain object list, dumped in <outdir>/dumps/<prefix>.obj.ldpdmp.tsv");
      System.err.println("\tRELATIONS  : the remaining arguments are paths to the relations files, dumped in <outdir>/relations/*.tsv");
      return;
    }

    ControlPathImporter imp = new ControlPathImporter(args[0]);

    System.out.print("[+] load mapping file " + args[1] + " ...");
    try {
      imp.loadMapping(args[1]);
    } catch (IOException e) {
        System.out.println("\n[!] failed to load mapping file: " + e.getMessage());
    }

    for (i=2; i<args.length; i++) {
      System.out.print("[+] importing file " + args[i] + " ...");
      try {
        imp.doImport(args[i]);
      } catch (IOException e) {
        System.out.println("\n[!] failed to import file: " + e.getMessage());
      }
    }

    System.out.println("[+] shutdown...");
    imp.shutdown();
    System.out.println("[+] done: " + imp.importedNodes + " node(s) / " + imp.importedRels + " relationship(s)");
  }
}
