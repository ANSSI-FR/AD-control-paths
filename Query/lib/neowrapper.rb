#!/usr/bin/env ruby
# encoding: utf-8

require 'neography'
require 'json'
require 'set'

Neography.configure do |config|
  config.protocol       = "http://"
  config.server         = "127.0.0.1"
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

module Enumerable
  def intersect?(other)
    other.any? {|v| h[v] }
  end
end

class NeoWrapper
  attr_accessor :maxdepth, :color, :verbose, :denyace

  def initialize()
    @neo = Neography::Rest.new( { :username => "neo4j", :password => "secret" } )
    @maxdepth = 20
    @color = true
    @verbose = false
    @nodecache = {}
    @relcache = {}
  end

  def set_password(passwd)
    @neo = Neography::Rest.new( { :username => "neo4j", :password => passwd } )
  end

  def debug(msg)
    return unless @verbose
    s = ""
    s << "\e[1;33m" if @color
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
    s << "\e[31m" if @color
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

  def label(name)
    query = ""
    query << "MATCH (n) "
    query << "WHERE n.name = { name } "
    query << "RETURN DISTINCT labels(n)"

    debug "cypher query: #{query}"
    return @neo.execute_query(query, { :name => name })["data"].first.first rescue nil
  end

  # node
  # direction: :from, :to
  # path_type: :full, :short
  def control_graph(node, direction, path_type)
    nodes = control_nodes(node, direction, nil)
    paths = query_path(node, direction, nodes)
    debug "found #{nodes.size} nodes and #{paths.size - 1} paths, creating graph"

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



    nodes_withrel = []
    rels_type = {}
    rels.each do |r|
      s, e = relstart(r), relend(r)
      nodes_withrel |= [s]
      nodes_withrel |= [e]
      rels_type[ [s, e] ] ||= []
      rels_type[ [s, e] ] << reltype(r)
    end
    
    nodes_withrel.each do |n|
      h["nodes"] << {
        "id" => n,
        "name" => nodename(n),
        "type" => nodetype(n),
        "shortname" => nodename(n).split(/,/).first,
        "dist" => path_len[n]
      }
      nodes_idx << n
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
    paths = Marshal.load(Marshal.dump(query_path(node, direction)))
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

    info "found #{paths.size-1} validated path(s)" # do not count the 0-length path
    return paths
  end

  # node
  # direction: :from, :to
  # type: type of node ("group", "user", ..., nil means no type filter)
  def control_nodes(node, direction, label)
    if @prev_cn_node == node && @prev_cn_direction == direction && @prev_cn_label == label
	  return @prev_control_nodes
	end
	@prev_cn_node = node
	@prev_cn_direction = direction
	@prev_cn_label = label
	
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
	  @prev_control_nodes = nodes
      return @prev_control_nodes
    end

    # filter by label
    debug "filtering nodes (label=#{label})"
    query = ""
    query << "MATCH (n:#{label}) " # labels cannot be passed as parameters
    query << "WHERE id(n) IN { id } "
    query << "RETURN DISTINCT id(n)"

    debug "cypher query: #{query} (#id=#{nodes.size})"
    @prev_control_nodes = @neo.execute_query(query, { :id => nodes })["data"].map { |n| n.first }
	return @prev_control_nodes
  end

  def nodename(n)
    @nodecache[n] ||= {}
    @nodecache[n]["name"] ||= @neo.get_node_properties(n, "name")["name"]
  end

  def nodetype(n)
    @nodecache[n] ||= {}
    if nodename(n).count('@') == 1 and nodename(n).count('=') == 0
      @nodecache[n]["type"] ||= 'email'
    else
      @nodecache[n]["type"] ||= @neo.get_node_labels(n).last # should have only one label
    end
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
    if @prev_qp_node == node && @prev_qp_direction == direction && @prev_qp_control == control then
	    return @prev_query_path
	  end
	  @prev_qp_node = node
	  @prev_qp_direction = direction	
    control ||= control_nodes(node, direction, nil)
	  @prev_qp_control = control
	
    query = ""    
    case direction
    when :from
      # no need to check for maxlength, as control_nodes (above) only returns
      # nodes within reach
      query << "MATCH path = allShortestPaths( (n)-[r*]->(control) ) "
    when :to
      query << "MATCH path = allShortestPaths( (n)<-[r*]-(control) ) "
    else
      raise "unknown direction: #{direction}"
    end  
    query << "WHERE id(n) = { n } "
    query << "AND id(control) IN { control } "
    if nodename(node).count('@') == 0
      query << "AND NONE( rel in r WHERE type(rel)='RBAC_SET_MBX' OR type(rel)='RBAC_ADD_MBXPERM' \
      OR type(rel)='RBAC_ADD_MBXFOLDERPERM' OR type(rel)='RBAC_SET_MBXFOLDERPERM' OR type(rel)='RBAC_CONNECT_MBX' \
      OR type(rel)='RBAC_NEW_MBXEXPORTREQ' \
      ) "
    end
    query << "RETURN path"

    debug "cypher query: #{query} (#control=#{control.size})"
    if @denyace then
      @prev_query_path = @neo.execute_query(query, { :control => control, :n => node })["data"].flatten
	    info "found #{@prev_query_path.size - 1} unfiltered control path(s)"
      denied_paths = validate_paths(@prev_query_path,direction)
      info "denied #{denied_paths.size} path(s)"
      debug "denied_paths: #{denied_paths}"
# denied_paths indexes are naturally growing down
      denied_paths.map {|i| @prev_query_path.delete_at(i)}
    else
      error "WARNING: --denyacefile was not defined"
      @prev_query_path = @neo.execute_query(query, { :control => control, :n => node })["data"].flatten
    end
	return @prev_query_path
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
    query << "WHERE id(n) IN { id } "
    query << "AND NOT id(neighbours) IN { ignore } " if ignore
    query << "RETURN DISTINCT id(neighbours)"

    debug "cypher query: #{query} (#id=#{nodes.size} #ignore=#{ignore.size})"
    return @neo.execute_query(query, { :id => nodes, :ignore => ignore })["data"].map { |n| n.first }
  end

  def all_rels_between(nodes)
    query = ""
    query << "MATCH (n)-[r]->(m) "
    query << "WHERE id(n) IN { id } AND id(m) IN { id } "
    query << "RETURN DISTINCT id(r)"
    debug "cypher query: #{query} (#id=#{nodes.size})"
    return @neo.execute_query(query, { :id => nodes })["data"].flatten
  end

  def id_from_url(u)
    return u.split(/\//).last.to_i
  end
  
  # Take into account deny ACE
  # in last relation of path and also
  # with group membership transitivity
  def validate_paths(found_paths,direction)
    info "filtering with #{denyace.size} denied ACE(s)"
    deniedpaths = []
	paths = Marshal.load(Marshal.dump(found_paths))
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
    info "building denyace hash table"
	h = Hash.new { false }
    denyace.each {|v| h[v] = true }
  debug "#{h}"
	transitiveRelations = ["GROUP_MEMBER","PRIMARY_GROUP","SID_HISTORY"]
  
  nonAceRelations = ["GROUP_MEMBER","PRIMARY_GROUP","SID_HISTORY",\
  "CONTAINER_HIERARCHY","GPLINK","AD_OWNER","SYSVOL_OWNER"]

	info "examining each path"
	paths.map! {|p| p.split("\t")}
	paths.map.with_index do |p,index| 
	  path_exploded = []
	  if @verbose
      stringPath = p.join("->")  
    end
    
	  while p.length > 1 do
      if nonAceRelations.include?(p[-2])
        p.pop(2)
        next
      end
	    path_exploded_end = []
      path_exploded_end << p[-3] << p[-1] << p[-2]
	    path_exploded << path_exploded_end
	    i = 2
	    while transitiveRelations.include?(p[-2 * i]) do
        path_exploded_item = []
	      path_exploded_item << p[-2 * i - 1] << p[-1] << p[-2]
	   	  path_exploded << path_exploded_item
		    i = i + 1
	    end
	    p.pop(2)
	  end
#    debug "#{path_exploded}"
	  if (path_exploded.any? {|v| h[v] })
	    debug "#{stringPath}"
	    deniedpaths.insert(0,index)
      print "-"
      $stdout.flush
	  else
	    print "+"
      $stdout.flush
	  end
	end
	puts "."
	return deniedpaths
  end 
end
