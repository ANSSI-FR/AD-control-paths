#!/usr/bin/env ruby
# encoding: utf-8

require 'neography'
require 'json'

Neography.configure do |config|
  config.protocol       = "http://"
  config.server         = "localhost"
  config.port           = 7474
  config.directory      = ""  # prefix this path with '/'
  config.cypher_path    = "/cypher"
  config.gremlin_path   = "/ext/GremlinPlugin/graphdb/execute_script"
  config.log_file       = "neography.log"
  config.log_enabled    = false
  config.max_threads    = 20
  config.authentication = nil  # 'basic' or 'digest'
  config.username       = nil
  config.password       = nil
  config.parser         = MultiJsonParser
end

class NeoWrapper
  attr_accessor :maxdepth, :color, :verbose

  def initialize()
    @neo = Neography::Rest.new
    @maxdepth = 20
    @color = false
    @verbose = false
    @nodecache = {}
    @relcache = {}
  end

  def debug(msg)
    return unless @verbose
    s = ""
    s << "\e[35m" if @color
    s << "[ ] "
    s << "#{msg}"
    s << "\e[0m" if @color
    puts s
  end
  def info(msg)
    s = ""
    s << "\e[32m" if @color
    s << "[+] "
    s << "#{msg}"
    s << "\e[0m" if @color
    puts s
  end
  def error(msg)
    s = ""
    s << "\e[33m" if @color
    s << "[!] "
    s << "#{msg}"
    s << "\e[0m" if @color
    puts s
  end

  def search(name)
    query = ""
    query << "MATCH (n) "
    query << "WHERE n.name =~ { regexp } "
    query << "RETURN DISTINCT n.name AS name"

    debug "cypher query: #{query}"
    return @neo.execute_query(query, { :regexp => ".*#{name}.*" })["data"].map { |n| n.first }
  end

  def id(name)
    query = ""
    query << "MATCH (n) "
    query << "WHERE n.name = { name } "
    query << "RETURN DISTINCT id(n)"

    debug "cypher query: #{query}"
    return @neo.execute_query(query, { :name => name })["data"].first.first rescue nil
  end


  # node
  # direction: :from, :to
  # path_type: :full, :short
  def control_graph(node, direction, path_type)
    nodes = control_nodes(node, direction, nil)
    paths = query_path(node, direction, nodes)

    debug "found #{nodes.size} nodes and #{paths.size} paths, creating graph"

    case path_type
    when :full
      rels = all_rels_between(nodes)
    when :short
      rels = []
    else
      raise "unknown graph type #{path_type}"
    end

    # get path length for all controlling nodes
    path_len = {}
    paths.each do |p|
      rels << p["relationships"].map { |r| id_from_url(r) }
      path_len[ id_from_url(p["end"]) ] = p["length"]
    end

    rels.flatten!
    rels.uniq!

    h = { "nodes" => [], "links" => [] }
    nodes_idx = []

    nodes.each do |n|
      h["nodes"] << {
        "id" => n,
        "name" => nodename(n),
        "type" => nodetype(n),
        "shortname" => nodename(n).split(/,/).first,
        "dist" => path_len[n]
      }
      nodes_idx << n
    end

    rels_type = {}
    rels.each do |r|
      s, e = relstart(r), relend(r)
      rels_type[ [s, e] ] ||= []
      rels_type[ [s, e] ] << reltype(r)
    end

    rels_type.each do |k, v|
      s = nodes_idx.index(k[0])
      e = nodes_idx.index(k[1])
      next unless s
      next unless e
      h["links"] << {
        "source" => s,
        "target" => e,
        "rels" => v
      }
    end

    info "control graph with #{h["nodes"].size} nodes and #{h["links"].size} links"
    return JSON.pretty_generate(h)
  end

  # node
  # direction: :from, :to
  # path_type: :full, :short
  def control_paths(node, direction)
    paths = query_path(node, direction)

    paths.map! do |p|
      p["nodes"].map! { |n| nodename(n) }
      p["relationships"].map! { |r| reltype(r) }
      case direction
      when :to
        p["nodes"].zip(p["relationships"]).flatten.compact.reverse.join("\t")
      when :from
        p["nodes"].zip(p["relationships"]).flatten.compact.join("\t")
      else
        raise "unknown direction: #{direction}"
      end
    end

    info "found #{paths.size-1} path(s)" # do not count the 0-length path
    return paths.join("\n")
  end

  # node
  # direction: :from, :to
  # type: type of node ("group", "user", ..., nil means no type filter)
  def control_nodes(node, direction, label)
    nodes = []
    nodes[0] = [node]
    i = 0

    while true do
      if i >= @maxdepth
        error "reached maxdepth (#{maxdepth}), stopping search"
        break
      end
      nodes[i+1] = []
      nodes[i+1] << neighbours(nodes[i], direction, nodes.flatten)
      nodes[i+1].flatten!
      break if nodes[i+1].size == 0
      i = i+1
    end
    nodes.flatten!
    info "found #{nodes.size} control nodes, max depth is #{i}"

    if label.nil?
      return nodes
    end

    # filter by label
    debug "filtering nodes (label=#{label})"
    query = ""
    query << "MATCH (n:#{label}) " # labels cannot be passed as parameters
    query << "WHERE id(n) = { id } "
    query << "RETURN DISTINCT id(n)"

    debug "cypher query: #{query} (#nodes=#{nodes.size})"
    return @neo.execute_query(query, { :id => nodes })["data"].map { |n| n.first }
  end

  def nodename(n)
    @nodecache[n] ||= {}
    @nodecache[n]["name"] ||= @neo.get_node_properties(n, "name")["name"]
  end

  def nodetype(n)
    @nodecache[n] ||= {}
    @nodecache[n]["type"] ||= @neo.get_node_labels(n).first # should have only one label
  end

  def reltype(r)
    @relcache[r] ||= {}
    @relcache[r]["type"] ||= @neo.get_relationship(r)["type"]
  end

  def relstart(r)
    @relcache[r] ||= {}
    @relcache[r]["start"] ||=  id_from_url(@neo.get_relationship(r)["start"])
  end
  def relend(r)
    @relcache[r] ||= {}
    @relcache[r]["end"] ||= id_from_url(@neo.get_relationship(r)["end"])
  end

  private

  def query_path(node, direction, control = nil)
    control ||= control_nodes(node, direction, nil)

    query = ""
    case direction
    when :from
      # no need to check for maxlength, as control_nodes (above) only returns
      # nodes within reach
      query << "MATCH path = allShortestPaths( (n)-->(control) ) "
    when :to
      query << "MATCH path = allShortestPaths( (n)<--(control) ) "
    else
      raise "unknown direction: #{direction}"
    end
    query << "WHERE id(n) = { n } "
    query << "AND id(control) = { control } "
    query << "RETURN path"

    debug "cypher query: #{query} (#control=#{control.size})"
    return @neo.execute_query(query, { :control => control, :n => node })["data"].flatten
  end

  # return ids
  def neighbours(nodes, direction, ignore)
    query = ""
    case direction
    when :from
      query << "MATCH (n)-->(neighbours) "
    when :to
      query << "MATCH (n)<--(neighbours) "
    else
      raise "unknown direction: #{direction}"
    end
    query << "WHERE id(n) = { id } "
    query << "AND NOT id(neighbours) IN { ignore } " if ignore
    query << "RETURN DISTINCT id(neighbours)"

    debug "cypher query: #{query} (#id=#{nodes.size} #ignore=#{ignore.size})"
    return @neo.execute_query(query, { :id => nodes, :ignore => ignore })["data"].map { |n| n.first }
  end

  def all_rels_between(nodes)
    query = ""
    query << "MATCH (n)-[r]->(m) "
    query << "WHERE id(n) = { id } AND id(m) = { id } "
    query << "RETURN DISTINCT id(r)"
    debug "cypher query: #{query} (#id=#{nodes.size})"
    return @neo.execute_query(query, { :id => nodes })["data"].flatten
  end

  def id_from_url(u)
    return u.split(/\//).last.to_i
  end
end
