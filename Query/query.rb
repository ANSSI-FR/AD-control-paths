#!/usr/bin/env ruby
# encoding: utf-8

require 'getoptlong'
require 'fileutils'
require 'csv'
require 'rubygems'

$LOAD_PATH.unshift '.'

require_relative 'lib/neowrapper'
require_relative 'lib/defaults'

exit if Object.const_defined?(:Ocra) #allow ocra to create an exe without executing the entire script

opts = GetoptLong.new(
  [ '--help', '-h', GetoptLong::NO_ARGUMENT ],

  # eye candy
  [ '--color', '-c', GetoptLong::NO_ARGUMENT ],
  [ '--verbose', '-v', GetoptLong::NO_ARGUMENT ],

  # operation options
  [ '--type', '-t', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--direction', '-d', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--maxdepth', '-m', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--nodetype', '-n', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--lang', '-l', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--password', '-p', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--denyacefile', '-y', GetoptLong::REQUIRED_ARGUMENT ],

  # operations
  [ '--quick', '-Q', GetoptLong::OPTIONAL_ARGUMENT ],
  [ '--full', '-F', GetoptLong::OPTIONAL_ARGUMENT ],
  [ '--search', '-S', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--info', '-I', GetoptLong::REQUIRED_ARGUMENT ],
  [ '--graph', '-G', GetoptLong::OPTIONAL_ARGUMENT ],
  [ '--path', '-P', GetoptLong::OPTIONAL_ARGUMENT ],
  [ '--nodes', '-N', GetoptLong::OPTIONAL_ARGUMENT ]
)

infos = {}
w = NeoWrapper.new

def usage
  puts <<EOS
#{$0} [OPTIONS] [TARGET]

TARGET: node id or node name (can be searched from partial name, e.g. "cn=domain admins,")

OPTIONS:
  --help: this help
  
  --------------------------------------------------------------------------------
  GENERIC: automatic operations mode
  
    --quick FILENAME: create shortest control graphs/paths/nodes to "cn=domain admins,"
	                    in DIRECTORY (default is .\out\)
	
	  --full DIRECTORY: full audit mode (very long)
                      create control graphs/paths/nodes for many default targets (defined in lib/defaults.rb)
			          		  in DIRECTORY (default is .\out\)

  --------------------------------------------------------------------------------
  SPECIFIC OPERATIONS: specify the operation you want to perform
              default is --nodes

    --nodes FILE: output control nodes to FILE (or stdout if FILE is absent)
                  direction and type can be specified with --direction and --type options
                  nodes type can be filtered with --nodetype option

    --graph FILE: output a control graph to FILE (or stdout if FILE is absent)
                  direction and type can be specified with --direction and --type options
                  output is formated in JSON

    --path FILE: output control paths to FILE (or stdout if FILE is absent)
                 direction and type can be specified with --direction and --type options
                 output is formated in tab separated fields



    --search NAME: search for a node with the given NAME and print its ID
                   NAME is interpreted as a Java regexp, so special charaters (such as '{') need to be escaped, or they will be interpreted

    --info NAME: print information about the node with the given NAME
                 information include known nodetype, number of directly controlling and controlled nodes
                 NAME is also interpreted as a Java regexp

  --------------------------------------------------------------------------------
  FINE TUNING: tweak operations behaviour

  --lang LANG: when running in automatic-mode, search for targets in LANG (en, fr)

  --maxdepth MAX: maximum length for control paths
                  default is 20

  --type TYPE: graph or path type (use with --graph or --path)
               "short": all shortest paths for a controlling node
               "full": all controlling path (only for --graph)
               default is "short"

  --direction DIRECTION: "from" the control node, or "to" the control node
                         default is "to" (i.e TARGET is controlled)

  --nodetype TYPE: only outputs control nodes of type TYPE (use with --nodes)

  --password PASSWORD: use this password instead of default one ('secret') to
                       authenticate to the REST interface

  --denyacefile FILE: specify a file of denied ACE to filter paths and graphs where those ACE are present, including transitively through group_member relations

  --------------------------------------------------------------------------------
  EYE CANDY

  --color: colorize output

  --verbose: print debugging output
EOS

  exit 0
end

opts.each do |opt, arg|
  case opt
  when '--help'
    usage
  when '--denyacefile'
    w.info "loading deny ace files \'#{arg}\'"
    w.denyace = []
    arg.split(',').each do |denyfile|
      w.denyace += CSV.read(denyfile,{:encoding => 'utf-16le:utf-8'})[1 .. -1]
	  end
    w.debug "denied ace: \'#{w.denyace}\'"
  when '--password'
    w.info "setting REST client password to \'#{arg}\'"
    w.set_password arg

  when '--search'
    w.search(arg).each do |target|
      w.info "#{target} [#{w.id(target)}]"
    end
    exit 0

  when '--info'
    w.maxdepth = 1
    w.search(arg).each do |target|
      id = w.id(target)
      res = "#{target} [#{id}]"
      res << "\n\ttype: #{w.label(target)}"
      n_from = w.control_nodes(id, :from, nil)
      res << "\n\tdirectly controlling #{n_from.size} node(s)"
      n_to = w.control_nodes(id, :to, nil)
      res << "\n\tdirectly controlled by #{n_to.size} node(s)"
      puts res
    end
    exit 0


  when '--full'
    infos[:auto] = true
    infos[:outdir] = (arg.size > 0) ? arg : "out"
  when '--quick'
    infos[:quick] = true
	  infos[:auto] = false
	  infos[:outdir] = (arg.size > 0) ? arg : "out"
  when '--type'
    infos[:type] = arg.to_sym
  when '--direction'
    infos[:direction] = arg.to_sym
  when '--nodetype'
    infos[:label] = arg
  when '--lang'
    infos[:lang] = arg.to_sym
  when '--maxdepth'
    unless arg =~ /\d+/
      w.error "invalid maxdepth setting: #{arg}"
      usage
    end
    w.maxdepth = arg.to_i
  when '--graph'
    infos[:graph] = (arg.size > 0) ? File.open(arg, "w") : $stdout
  when '--path'
    infos[:path] = (arg.size > 0) ? File.open(arg, "w") : $stdout
  when '--nodes'
    infos[:nodes] = (arg.size > 0) ? File.open(arg, "w") : $stdout
  when '--color'
    w.color = true
  when '--verbose'
    w.verbose = true
  else
    usage
  end
end

if infos[:auto] || infos[:quick]
  infos[:lang] ||= :en
  w.color = true
  FileUtils.mkdir_p(infos[:outdir]) unless File.directory? infos[:outdir]
end

if infos[:auto] || infos[:quick]
  raise "unknown lang: #{infos[:lang]}" unless TRANSLATION_TABLE[infos[:lang]]
  w.info "running in automatic audit mode, lang=#{infos[:lang]}, outdir=#{infos[:outdir]}"
  DEFAULT_TARGETS.each do |name_sym, defaults|

    name = TRANSLATION_TABLE[infos[:lang]][name_sym]
    raise "no translation for symbol #{name_sym}" unless name
    w.debug "translation: #{name_sym} -> #{name}"
    target = w.search(name).first
    unless target.nil?
      w.info "control nodes for #{name}"
      w.debug "control nodes -> #{defaults[:direction]} #{name}"
      begin
        Dir.chdir infos[:outdir] do
          fnodes = File.open(name_sym.to_s + "_#{defaults[:direction]}_nodes.txt", "w")
          nodes = w.control_nodes(w.id(target), defaults[:direction], nil)
          fnodes.write( nodes.map { |n| w.nodename(n) }.sort.join("\n"))
          fnodes.write("\n")
          fnodes.close()
        end
      rescue RuntimeError, Neography::NeographyError => e
        w.error "control nodes failed: #{e.message}"
        puts e.backtrace.inspect
      end      

      w.info "control paths for #{name}"
      w.debug "control paths -> short #{defaults[:direction]} #{name}"
      begin
        Dir.chdir infos[:outdir] do
          fpaths = File.open(name_sym.to_s + "_#{defaults[:direction]}_short_paths.txt", "w")
          fpaths.write(w.control_paths(w.id(target), defaults[:direction]).join("\n"))
          fpaths.write("\n")
          fpaths.close()
        end
      rescue RuntimeError, Neography::NeographyError => e
        w.error "control path failed: #{e.message}"
        puts e.backtrace.inspect
      end

	  w.info "control graph for #{name}"
      [ :short, :full ].each do |t|
        w.debug "control graph -> #{t} #{defaults[:direction]} #{name}"
        begin
          Dir.chdir infos[:outdir] do
            fgraph = File.open(name_sym.to_s + "_#{defaults[:direction]}_#{t}.json", "w")
            fgraph.write(w.control_graph(w.id(target), defaults[:direction], t))
            fgraph.write("\n")
            fgraph.close()
          end
        rescue RuntimeError, Neography::NeographyError => e
          w.error "control graph failed: #{e.message}"
          puts e.backtrace.inspect
          next
        end
	    if infos[:quick]
	      break
	    end
      end

    else
      w.error "did not find #{name}"
    end
    if infos[:quick]
      break
    end
  end
  exit 0
end

infos[:direction] ||= :to # default direction
infos[:type] ||= :short # default path type

target = ARGV.first
unless target
  w.error "no target specified"
  usage
end
target = target.downcase
w.debug("target=#{target}")

case target
when /^\d+$/ # numeric target
  w.debug "#{target} is a node id"
  target_id = target.to_i
  begin
    w.info "node[#{target_id}] = \'#{w.nodename(target_id)}\'"
  rescue Neography::NodeNotFoundException
    w.error "did not find node with id #{target_id}"
    exit 1
  end
else # name, try to find corresponding node
  w.info "\'#{target}\' is a node name, looking for it..."
  target_id = w.id(target)
  unless target_id
    # not an exact name, try to find a corresponding node
    w.debug "no exact match for \'#{target}\', searching..."
    search_res = w.search(target)
    case search_res.size
    when 0
      w.error "did not find node with name matching #{target}"
      exit 1
    when 1
      target_id = w.id(search_res.first)
      w.debug "only one match for \'#{target}\': #{search_res.first} [#{target_id}]"
    else
      w.error "multiple nodes with name matching #{target}:"
      search_res.each do |t|
        w.info "#{t} [#{w.id(t)}]"
      end
      exit 1
    end
  end
end

unless infos[:graph] or infos[:path] or infos[:nodes]
  w.info "no operation specified, printing controlling nodes to stdout"
  infos[:nodes] = $stdout
end

if infos[:nodes]
  w.info "control nodes #{"with type #{infos[:label]} " if infos[:label]}#{infos[:direction]} node number #{target_id}"
  nodes = w.control_nodes(target_id, infos[:direction], infos[:label])
  begin
    infos[:nodes].write( nodes.map { |n| w.nodename(n) }.sort.join("\n"))
    infos[:nodes].write("\n")
    infos[:nodes].close
  rescue RuntimeError, Neography::NeographyError => e
    w.error "control graph failed: #{e.message}"
    puts e.backtrace.inspect
  end
end

if infos[:path]
  w.info "#{infos[:type]} control paths #{infos[:direction]} node number #{target_id}"
  begin
    infos[:path].write(w.control_paths(target_id, infos[:direction]).join("\n"))
    infos[:path].write("\n")
    infos[:path].close
  rescue RuntimeError, Neography::NeographyError => e
    w.error "control graph failed: #{e.message}"
    puts e.backtrace.inspect
  end
end

if infos[:graph]
  w.info "#{infos[:type]} control graph #{infos[:direction]} node number #{target_id}"
  begin
    infos[:graph].write(w.control_graph(target_id, infos[:direction], infos[:type]))
    infos[:graph].write("\n")
    infos[:graph].close
  rescue RuntimeError, Neography::NeographyError => e
    w.error "control graph failed: #{e.message}"
    puts e.backtrace.inspect
  end
end
