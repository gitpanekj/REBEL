/**
 * @file graph_types.hpp
 * @author Jan Pánek (xpanek11@stud.fit.vutbr.cz)
 * @brief Shared types used across graphlib.
 * @version 0.1
 * @date 2026-010-10
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef GRAPH_TYPES_H
#define GRAPH_TYPES_H

#include <cstdint>
#include <ostream>
#include <vector>

namespace graphlib {

using IdType = std::uint32_t;
using VertexId = IdType;
using EdgeId = IdType;
using offset_t = std::uint64_t;

/**
 * @brief Representation of an outgoing edge to a vertex in oriented graph.
 *
 * Representation of an outgoing edge to a vertex in oriented graph.
 * An item of adjacency list.
 */
struct OutgoingEdge {
  VertexId to; ///> id of the destination vertex
  EdgeId id;   ///> id of the edge (same as for corresponding ingoing edge)
  OutgoingEdge(VertexId to, EdgeId id) : to(to), id(id) {}
  OutgoingEdge() = default;

  bool operator==(const OutgoingEdge &) const = default;
  friend std::ostream &operator<<(std::ostream &os, const OutgoingEdge &e) {
    return os << "{id: " << e.id << ", to: " << e.to << "}";
  }
};

/**
 * @brief Representation of an ingoing edge from a vertex in oriented graph.
 *
 * Representation of an ingoing edge from a vertex in oriented graph.
 * An item of adjacency list.
 */
struct IngoingEdge {
  VertexId from; ///> id of the source vertex
  EdgeId id;     ///> id of the edge (same as for corresponding outgoing edge)
  IngoingEdge(VertexId from, EdgeId id) : from(from), id(id) {}
  IngoingEdge() = default;
  bool operator==(const IngoingEdge &) const = default;
  friend std::ostream &operator<<(std::ostream &os, const IngoingEdge &e) {
    return os << "{id: " << e.id << ", from: " << e.from << "}";
  }
};

/**
 * @brief Structure representing an oriented adge in a graph as a tuple (from,
 * to).
 */
struct EdgeEndpoints {
  VertexId from; ///> id of the source vertex
  VertexId to;   ///> id of the target vertex
  EdgeEndpoints(VertexId from, VertexId to) : from(from), to(to) {}
  EdgeEndpoints() = default;
  bool operator==(const EdgeEndpoints &) const = default;
  friend std::ostream &operator<<(std::ostream &os, const EdgeEndpoints &e) {
    return os << "{from: " << e.from << ", to: " << e.to << "}";
  }
};



template <typename R, typename ElementType>
concept RangeOf = std::ranges::input_range<R> &&
                  std::same_as<std::ranges::range_value_t<R>, ElementType>;

template <typename T>
concept GraphType = requires(T &obj, VertexId vid, EdgeId eid) {
  { obj.getSuccessors(vid) } -> RangeOf<VertexId>;
  { obj.getPredecessors(vid) } -> RangeOf<VertexId>;
  { obj.getEdgesBetween(vid, vid) } -> RangeOf<EdgeId>;
  { obj.getOutgoingEdges(vid) } -> RangeOf<OutgoingEdge>;
  { obj.getIngoingEdges(vid) } -> RangeOf<IngoingEdge>;
  { obj.getEdgeEndpoints(eid) } -> std::convertible_to<EdgeEndpoints>;
  { obj.getVertices() } -> RangeOf<VertexId>;
  { obj.getEdges() } -> RangeOf<EdgeId>;
  { obj.getEdgeNumber() } -> std::same_as<IdType>;
  { obj.getVertexNumber() } -> std::same_as<IdType>;
  { obj.markAsDeleted(eid)  } -> std::same_as<void>;
  { obj.markAsAvailable(eid) } -> std::same_as<void>;
  { obj.isEdgeVisible(eid) } -> std::same_as<bool>;
};


template <typename T>
struct Serializer;

template <typename T>
concept Serializable = std::is_trivially_copyable_v<T> ||
                       requires(const T &obj, std::vector<std::byte> &buffer,
                                std::span<std::byte> data) {
                         {
                           Serializer<T>::serialize(obj, buffer)
                         } -> std::convertible_to<offset_t>;
                         {
                           Serializer<T>::serialize(obj)
                         } -> std::same_as<std::vector<std::byte>>;
                         { Serializer<T>::deserialize(data) } -> std::same_as<T>;
                       };
} // namespace graphlib

#endif