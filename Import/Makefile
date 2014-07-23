all: check-neo4j-path
	javac -classpath "${NEO4J}/lib/*" ControlPathImporter.java

check-neo4j-path:
ifndef NEO4J
	$(error Usage: make NEO4J=/path/to/neo4j)
endif

clean:
	rm -f *.class
	rm -f *~
