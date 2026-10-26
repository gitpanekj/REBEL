/**
 * @file graph_topology.hpp
 * @author Jan Pánek (xpanek11@stud.fit.vutbr.cz)
 * @brief Header file for graph_topology.cpp
 * @version 0.1
 * @date 2026-09-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef GRAPH_TOPOLOGY
#define GRAPH_TOPOLOGY

#include "graph_types.hpp"
#include "memory_utils.hpp"
#include <ranges>
#include <string>
#include <vector>

namespace graphlib {

class DirectedGraphTopologyView;
class DirectedGraphTopology;

/**
* @brief A partially mutable view of serialized DirectedGraphTopology.
*
* The DirectedGraphTopologyView class is a partially mutable view of serialized
DirectedGraphTopology (see. @ref DirectedGraphTopology "text").
* It can be transformed back to fully mutable DirectedGraphTopology.
* It does not provide support for neither vertex nor edge labels (see. @ref
LabelStorage "text").
*
* The class provides interface for for traversal of the graph topology.
*
* Additionally an edge can be marked as temporarily removed.
* In such case, the edge is present in the according data structures
* but all the algorithms and access methods ignore the edge until
* it is marked as visible.

*
* Each vertex and edge is assigned a unique id.
* Each vertex is assigned a unique id in range [0, n_vertices).
* Each edge is assigned a unique id in range [0, n_edges).
*
* Memory layout is | Header | Payload | where header contains information about
* number of edges, vertices and relative offsets of individual parts of payload.
* Payload in turn consists of the following parts:
*
* outgoing_edge_offset_list: on position vertex_id contains offset of the outgoing edges
list for given vertex_id relative to the start of outgoing_edges
* outgoing_edges: flattened array of outgoing edges
* ingoing_edge_offset_list: on position vertex_id contains offset of the ingoing edges
list for given vertex_id relative to the start of ingoing_edges
* ingoing_edges: flattened array of ingoing edges
* edge_endpoints: on position edge_id contains edge endpoints of given edge
* visibility_mask: a bitmask representing visibility of an edge
*
*/
class DirectedGraphTopologyView {
public:
  struct alignas(64) GraphTopologyViewHeader {
    std::size_t n_vertices;
    std::size_t n_edges;

    /// Forward edges
    offset_t outgoing_edge_offset_list_offset;
    offset_t outgoing_edge_list_offset;

    // Backward edges
    offset_t ingoing_edge_offset_list_offset;
    offset_t ingoing_edge_list_offset;

    offset_t edge_endpoints_offset;

    offset_t visibility_mask_offset;
    GraphTopologyViewHeader() {
      n_vertices = 0;
      n_edges = 0;
      outgoing_edge_offset_list_offset = 0;
      outgoing_edge_list_offset = 0;
      ingoing_edge_offset_list_offset = 0;
      ingoing_edge_list_offset = 0;
      edge_endpoints_offset = 0;
      visibility_mask_offset = 0;
    }
  };

private:
  std::vector<std::byte> raw_bytes; ///> raw graph data

  const GraphTopologyViewHeader
      *header; ///> offset of header in the graph buffer

  std::span<const offset_t>
      outgoing_edge_offset_list; ///> offset for adjacencny lists for individual
                                 /// vertices in the outgoing_edge_span
  std::span<const OutgoingEdge> outgoing_edges;

  std::span<const offset_t>
      ingoing_edge_offset_list; ///> offset for adjacencny lists for individual
                                /// vertices in the ingoing_edge_span
  std::span<const IngoingEdge> ingoing_edges;

  std::span<const EdgeEndpoints> edge_endpoints;

  std::span<std::uint64_t> visibility_mask;

  inline bool isValidEdgeId(EdgeId id) const noexcept {
    return id < header->n_edges;
  }

  inline bool isValidVertexId(VertexId id) const noexcept {
    return id < header->n_vertices;
  }

  inline void validEdgeOrError(EdgeId id) const {
    if (!isValidEdgeId(id)) {
      throw std::invalid_argument("Given edge_id " + std::to_string(id) +
                                  " does not identify an edge.");
    }
  }

  inline void validVertexOrError(VertexId id) const {
    if (!isValidVertexId(id)) {
      throw std::invalid_argument("Given vertex_id " + std::to_string(id) +
                                  " does not identify a vertex.");
    }
  }

public:
  /**
   * @brief Construct a new Graph Topology View object
   *
   * @param data serialized graph
   */
  explicit DirectedGraphTopologyView(std::vector<std::byte> data) {
    // TODO: refactor so that the semantic and memoty layout is cleaner
    raw_bytes = data;
    header = offset_to_ptr<const GraphTopologyViewHeader>(raw_bytes.data(), 0);

    outgoing_edge_offset_list = std::span<const offset_t>(
        offset_to_ptr<const offset_t>(raw_bytes.data(),
                                      header->outgoing_edge_offset_list_offset),
        header->n_vertices + 1);

    outgoing_edges = std::span<const OutgoingEdge>(
        offset_to_ptr<const OutgoingEdge>(raw_bytes.data(),
                                          header->outgoing_edge_list_offset),
        header->n_edges);

    ingoing_edge_offset_list = std::span<const offset_t>(
        offset_to_ptr<const offset_t>(raw_bytes.data(),
                                      header->ingoing_edge_offset_list_offset),
        header->n_vertices + 1);

    ingoing_edges = std::span<const IngoingEdge>(
        offset_to_ptr<const IngoingEdge>(raw_bytes.data(),
                                         header->ingoing_edge_list_offset),
        header->n_edges);

    edge_endpoints = std::span<const EdgeEndpoints>(
        offset_to_ptr<const EdgeEndpoints>(raw_bytes.data(),
                                           header->edge_endpoints_offset),
        header->n_edges);

    const std::size_t bitmask_words = (header->n_edges + 63) / 64;
    visibility_mask = std::span<std::uint64_t>(
        offset_to_ptr<std::uint64_t>(raw_bytes.data(),
                                     header->visibility_mask_offset),
        bitmask_words);
  }

  DirectedGraphTopologyView() = delete;

  /**
   * @brief Get edges between nodes src and dst.
   *
   * If either of src or dst does not exist an empty view is returned.
   *
   * @param src source vertex id
   * @param dst destination vertex id
   * @return view of OutgoingEdges between nodes src and dst
   */
  [[nodiscard]] auto getEdgesBetween(VertexId src, VertexId dst) const {
    return getOutgoingEdges(src) |
           std::views::filter(
               [dst](const OutgoingEdge &edge) { return edge.to == dst; }) |
           std::views::transform(
               [](const OutgoingEdge &edge) -> VertexId { return edge.id; });
  }

  /**
   * @brief Get outgoing edges of vertex src.
   *
   * If src does not exist an empty span is returned.
   *
   * @param src id of source vertex
   * @return span of outgoing edges from vertex src
   */
  [[nodiscard]] std::span<const OutgoingEdge>
  getOutgoingEdges(VertexId src) const {
    validVertexOrError(src);
    const offset_t start = outgoing_edge_offset_list[src];
    const offset_t count = outgoing_edge_offset_list[src + 1] - start;
    return outgoing_edges.subspan(start, count);
  }

  /**
   * @brief Get ingoing edges of vertex dst
   *
   * If dst does not exist an empty vector is returned.
   *
   * @param dst id of destination vertex
   * @return span of ingoing edges to vertex dst
   */
  [[nodiscard]] std::span<const IngoingEdge>
  getIngoingEdges(VertexId dst) const {
    validVertexOrError(dst);
    const offset_t start = ingoing_edge_offset_list[dst];
    const offset_t count = ingoing_edge_offset_list[dst + 1] - start;
    return ingoing_edges.subspan(start, count);
  }

  /**
   * @brief Get successor vertices of vertex src.
   *
   * If src does not exist an empty vector is returned.
   *
   * @param src id of source vertex
   * @return vector of successors
   */
  [[nodiscard]] auto getSuccessors(VertexId src) const {
    return getOutgoingEdges(src) |
           std::views::transform(
               [](const OutgoingEdge &edge) -> VertexId { return edge.to; });
  }

  /**
   * @brief Get vector of predecessor vertices of vertex src.
   *
   * If dst does not exist an empty vector is returned.
   *
   * @param dst id of destination vertex
   * @return vector of predecessors
   */
  [[nodiscard]] auto getPredecessors(VertexId dst) const {
    return getIngoingEdges(dst) |
           std::views::transform(
               [](const IngoingEdge &edge) -> VertexId { return edge.from; });
  }

  /**
   * @brief Mark an edge as temporarily deleted.
   *
   * Temporarily removed edges are ignored during graph traversal and
   * in provided graph algorithms.
   *
   * If edge_id does not exist, nothing happens.
   *
   * @param edge_id Temporarily removed edge.
   */
  void markAsDeleted(EdgeId edge_id) {
    validEdgeOrError(edge_id);
    const std::uint64_t deletion_mask = ~(1UL << (edge_id % 64));
    visibility_mask[edge_id / 64] &= deletion_mask;
  }

  /**
   * @brief Mark an edge as available.
   *
   * Available edges are NOT ignored during graph traversal and
   * in provided graph algorithms.
   *
   * If edge_id does not exist, nothing happens.
   *
   * @param edge_id An edge which is made available.
   */
  void markAsAvailable(EdgeId edge_id) {
    validEdgeOrError(edge_id);
    const std::uint64_t marking_mask = 1UL << (edge_id % 64);
    visibility_mask[edge_id / 64] |= marking_mask;
  }

  /**
   * @brief Get endpoints of an edge
   *
   * @param edge_id id of an edge
   * @return Endpoints of the edge edge_id.
   */
  [[nodiscard]] EdgeEndpoints getEdgeEndpoints(int edge_id) const {
    validEdgeOrError(edge_id);
    return edge_endpoints[edge_id];
  }

  /**
   * @brief Get endpoints of an edge
   *
   * @param edge_id id of an edge
   * @return Endpoints of the edge edge_id.
   */
  [[nodiscard]] std::span<const EdgeEndpoints>
  getAllEdgeEndpoints() const noexcept {
    return edge_endpoints;
  }

  /**
   * @brief Determine whether and egde is currently visible.
   *
   */
  [[nodiscard]] bool isEdgeVisible(EdgeId edge_id) const {
    validEdgeOrError(edge_id);
    const std::size_t word = visibility_mask[edge_id / 64];
    const std::uint64_t lookup_mask = 1UL << (edge_id % 64);
    return (word & lookup_mask) != 0;
  }

  [[nodiscard]] auto getVertices() const noexcept {
    return std::views::iota(VertexId{0},
                            static_cast<VertexId>(header->n_vertices));
  }

  [[nodiscard]] auto getEdges() const noexcept {
    return std::views::iota(EdgeId{0}, static_cast<EdgeId>(header->n_edges)) |
           std::views::filter(
               [this](EdgeId id) noexcept { return this->isEdgeVisible(id); });
  }

  IdType getEdgeNumber() const { return header->n_edges; }
  IdType getVertexNumber() const { return header->n_vertices; }
}; // End of DirectedGraphTopologyView

/**
 * @brief A mutable representation of a toplogy of a directed graph.
 *
 * The DirectedGraphTopology class is a mutable representation of a topology of
 * a directed graph. It does not provide support for neither vertex nor edge
 * labels (see. @ref LabelStorage "text").
 *
 * The class provides interface for building the topology (add and remove
 * edge/vertex) for traversal of the graph topology and serialization to binary
 * format (see. @ref DirectedGraphTopologyView).
 *
 * Additionally an edge can be marked as temporarily removed.
 * In such case, the edge is present in the according data structures
 * but all the algorithms and access methods ignore the edge until
 * it is marked as visible.
 *
 * Each vertex and edge is assigned a unique id.
 * The id corresponds to the current value of a vertex counter or an edge
 * counter respectively. The implementation guarantees that at every moment the
 * value of according counter always represents the number of vertices/edges.
 * I.e. each vetex/edge is assigned a unique int form range [0, counter) none of
 * them is unassigned.
 *
 * The implementation is otimized for adding new vertices and edges and toplogy
 * traversal. NOTE that deletion of edges and vertices is possible but expensive
 * since it involves extensive memory moves.
 */
class DirectedGraphTopology {
private:
  std::vector<std::vector<OutgoingEdge>>
      outgoing_edges; ///< Outoging edge adjacency vector
  std::vector<std::vector<IngoingEdge>>
      ingoing_edges;                         ///< Ingoing edge adjacency vector
  std::vector<EdgeEndpoints> edge_endpoints; ///< Edge pair vector
  std::vector<bool> visibility_mask;         ///< Visibility mask
  EdgeId edge_id_counter = 0;                ///< edge id counter
  VertexId vertex_id_counter = 0;            ///< vertex id counter

  inline bool isValidEdgeId(EdgeId id) const noexcept {
    return id < edge_id_counter;
  }

  inline bool isValidVertexId(VertexId id) const noexcept {
    return id < vertex_id_counter;
  }

  inline void validEdgeOrError(EdgeId id) const {
    if (!isValidEdgeId(id)) {
      throw std::invalid_argument("Given edge_id " + std::to_string(id) +
                                  " does not identify an edge.");
    }
  }

  inline void validVertexOrError(VertexId id) const {
    if (!isValidVertexId(id)) {
      throw std::invalid_argument("Given vertex_id " + std::to_string(id) +
                                  " does not identify a vertex.");
    }
  }

public:
  DirectedGraphTopology() = default;
  explicit DirectedGraphTopology(DirectedGraphTopologyView &view) {
    edge_id_counter = view.getEdgeNumber();
    vertex_id_counter = view.getEdgeNumber();

    edge_endpoints = std::vector<EdgeEndpoints>(edge_id_counter);
    auto edge_endpoints_view = view.getAllEdgeEndpoints();
    std::copy(edge_endpoints_view.begin(), edge_endpoints_view.end(),
              edge_endpoints.begin());

    visibility_mask = std::vector<bool>(edge_id_counter);
    for (size_t i = 0; i < edge_id_counter; i++)
      visibility_mask[i] = view.isEdgeVisible(i);

    outgoing_edges = std::vector<std::vector<OutgoingEdge>>(vertex_id_counter);
    for (size_t i = 0; i < vertex_id_counter; i++) {
      auto out = view.getOutgoingEdges(i);
      std::copy(out.begin(), out.end(), outgoing_edges[i].begin());
    }

    ingoing_edges = std::vector<std::vector<IngoingEdge>>(vertex_id_counter);
    for (size_t i = 0; i < vertex_id_counter; i++) {
      auto in = view.getIngoingEdges(i);
      std::copy(in.begin(), in.end(), ingoing_edges[i].begin());
    }
  }

  /**
   * @brief Add a new vertex to the graph
   *
   * Returned vertex id corresponds to the number of vertices in the graph.
   *
   * @return VertexId id of the created vertex
   */
  VertexId addVertex() {
    // Update id_counter
    int vertex_id = vertex_id_counter++;

    // Update adjacency list
    outgoing_edges.emplace_back();
    ingoing_edges.emplace_back();

    return vertex_id;
  }

  /**
   * @brief Add an orineted edge to the graphA
   *
   * Returned edge id corresponds to the number of edges in the graph.
   *
   * @param src source vertex id
   * @param dst destination vertex id
   * @return EdgeId id of the created edge
   */
  EdgeId addEdge(VertexId src, VertexId dst) {
    validVertexOrError(src);
    validVertexOrError(dst);

    int edge_id = edge_id_counter++;

    outgoing_edges[src].emplace_back(dst, edge_id);
    ingoing_edges[dst].emplace_back(src, edge_id);
    edge_endpoints.emplace_back(src, dst);
    visibility_mask.emplace_back(true);

    return edge_id;
  }

  // removeVertex is not requiread for now, may be implemented later
  // NOTE: the implementation would require label storage update as well -
  // manual NOTE: in ordet not to break the rest of the implementation, the
  // remove vertex must
  //       preserve invariant that vertex id is a total number of vertifces
  //       i.e. the vertex with the highest is must be swapped with the deleted
  //       one
  // void removeVertex();

  // removeEdge is not required for now, may be implemented later
  // NOTE: the implementation would require label storage update as well -
  // manual void removeEdge(VertexId src, VertexId dst);

  [[nodiscard]] auto getVertices() const noexcept {
    return std::views::iota(VertexId{0},
                            static_cast<VertexId>(vertex_id_counter));
  }

  [[nodiscard]] auto getEdges() const noexcept {
    return std::views::iota(EdgeId{0}, static_cast<EdgeId>(edge_id_counter)) |
           std::views::filter(
               [this](EdgeId id) noexcept { return this->isEdgeVisible(id); });
  }

  IdType getEdgeNumber() const { return edge_id_counter; }
  IdType getVertexNumber() const { return vertex_id_counter; }
  IdType getVertexIdCounter() const { return vertex_id_counter; }
  IdType getEdgeIdCounter() const { return edge_id_counter; }

  /**
   * @brief Get outgoing edges of vertex src.
   *
   * If src does not exist an empty span is returned.
   *
   * @param src id of source vertex
   * @return span of outgoing edges from vertex src
   */
  [[nodiscard]] std::span<const OutgoingEdge>
  getOutgoingEdges(VertexId src) const {
    validVertexOrError(src);
    return std::span<const OutgoingEdge>(outgoing_edges[src].cbegin(),
                                         outgoing_edges[src].cend());
  }

  /**
   * @brief Get ingoing edges of vertex dst
   *
   * If dst does not exist an empty vector is returned.
   *
   * @param dst id of destination vertex
   * @return span of ingoing edges to vertex dst
   */
  [[nodiscard]] std::span<const IngoingEdge>
  getIngoingEdges(VertexId dst) const {
    validVertexOrError(dst);
    return std::span<const IngoingEdge>(ingoing_edges[dst].cbegin(),
                                        ingoing_edges[dst].cend());
  }

  /**
   * @brief Get edges between nodes src and dst.
   *
   * If either of src or dst does not exist an empty view is returned.
   *
   * @param src source vertex id
   * @param dst destination vertex id
   * @return view of OutgoingEdges between nodes src and dst
   */
  [[nodiscard]] auto getEdgesBetween(VertexId src, VertexId dst) const {
    return getOutgoingEdges(src) |
           std::views::filter(
               [dst](const OutgoingEdge &edge) { return edge.to == dst; }) |
           std::views::transform(
               [](const OutgoingEdge &edge) -> VertexId { return edge.id; });
  }

  /**
   * @brief Get successor vertices of vertex src.
   *
   * If src does not exist an empty vector is returned.
   *
   * @param src id of source vertex
   * @return vector of successors
   */
  [[nodiscard]] auto getSuccessors(VertexId src) const {
    return getOutgoingEdges(src) |
           std::views::transform(
               [](const OutgoingEdge &edge) -> VertexId { return edge.to; });
  }

  /**
   * @brief Get vector of predecessor vertices of vertex src.
   *
   * If dst does not exist an empty vector is returned.
   *
   * @param dst id of destination vertex
   * @return vector of predecessors
   */
  [[nodiscard]] auto getPredecessors(VertexId dst) const {
    return getIngoingEdges(dst) |
           std::views::transform(
               [](const IngoingEdge &edge) -> VertexId { return edge.from; });
  }

  /**
   * @brief Mark an edge as temporarily deleted.
   *
   * Temporarily removed edges are ignored during graph traversal and
   * in provided graph algorithms.
   *
   * If edge_id does not exist, nothing happens.
   *
   * @param edge_id Temporarily removed edge.
   */
  void markAsDeleted(EdgeId edge_id) {
    validEdgeOrError(edge_id);
    if (edge_id >= edge_id_counter) [[unlikely]]
      return;
    visibility_mask[edge_id] = false;
  }

  /**
   * @brief Mark an edge as available.
   *
   * Available edges are NOT ignored during graph traversal and
   * in provided graph algorithms.
   *
   * If edge_id does not exist, nothing happens.
   *
   * @param edge_id An edge which is made available.
   */
  void markAsAvailable(EdgeId edge_id) {
    validEdgeOrError(edge_id);
    visibility_mask[edge_id] = true;
  }

  /**
   * @brief Get endpoints of an edge
   *
   * @param edge_id id of an edge
   * @return Endpoints of the edge edge_id.
   */
  [[nodiscard]] EdgeEndpoints getEdgeEndpoints(int edge_id) const {
    validEdgeOrError(edge_id);
    return edge_endpoints[edge_id];
  }

  /**
   * @brief Determine whether and egde is currently visible.
   *
   */
  [[nodiscard]] bool isEdgeVisible(EdgeId edge_id) const /*  */ {
    validEdgeOrError(edge_id);
    return visibility_mask[edge_id];
  }
}; // End of DirectedGraphTopology

} // namespace graphlib

#endif