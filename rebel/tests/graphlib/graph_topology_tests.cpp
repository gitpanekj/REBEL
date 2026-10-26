#include <cstddef>
#include <gtest/gtest.h>
#include <initializer_list>
#include <iostream>
#include <ostream>
#include <sstream>

#include "graphlib/graph_topology.hpp"
#include "graphlib/graph_types.hpp"
#include "graphlib/memory_utils.hpp"
#include "graphlib/serialization.hpp"
#include "graphlib/algorithms.hpp"

using namespace graphlib;

template <typename T>
bool areSame(std::vector<T> &v, std::initializer_list<T> l) {
  if (v.size() != l.size())
    return false;
  auto it1 = v.begin();
  auto it2 = l.begin();
  while (it1 != v.end()) {
    if (*it1 != *it2)
      return false;
    it1++, it2++;
  }
  return true;
}

std::string dump(const std::vector<IngoingEdge> &v) {
  std::ostringstream os;
  os << "[size: " << v.size() << "] ";
  for (const auto &e : v)
    os << e << ", ";
  return os.str();
}

std::string dump(const std::vector<OutgoingEdge> &v) {
  std::ostringstream os;
  os << "[size: " << v.size() << "] ";
  for (const auto &e : v)
    os << e << ", ";
  return os.str();
}

std::string dump(const std::vector<VertexId> &v) {
  std::ostringstream os;
  os << "[size: " << v.size() << "] ";
  for (const auto &e : v)
    os << e << ", ";
  return os.str();
}

TEST(GraphTopology, EmptyGraph) {
  // Arrange
  DirectedGraphTopology graph{};

  // Act
  auto serialized_graph = serialize(graph);
  DirectedGraphTopologyView graph_view{serialized_graph};

  std::vector<VertexId> vertices(std::from_range, graph_view.getVertices());
  std::vector<EdgeId> edges(std::from_range, graph_view.getEdges());

  // Assert
  ASSERT_EQ(
      serialized_graph.size(),
      paddToMultipleOf(sizeof(DirectedGraphTopologyView::GraphTopologyViewHeader), 64) +
          128 // 128 per 2 empty offset lists with sentinels only
      )
      << "Invalid size:" << serialized_graph.size();

  ASSERT_EQ(vertices.size(), 0);
  ASSERT_EQ(edges.size(), 0);
}

TEST(GraphTopology, NonEmptyGraph1) {

  // Arrange
  DirectedGraphTopology graph{};
  for (int i = 0; i < 9; i++)
    graph.addVertex();
  EdgeEndpoints edges[] = {
      EdgeEndpoints{0, 1}, EdgeEndpoints{1, 2}, EdgeEndpoints{1, 3},
      EdgeEndpoints{2, 3}, EdgeEndpoints{3, 4}, EdgeEndpoints{4, 5},
      EdgeEndpoints{5, 6}, EdgeEndpoints{6, 4}, EdgeEndpoints{6, 7},
      EdgeEndpoints{7, 6}, EdgeEndpoints{4, 8}};
  for (const auto &edge : edges)
    graph.addEdge(edge.from, edge.to);

  // Act
  auto serialized_graph = serialize(graph);
  DirectedGraphTopologyView graph_view{serialized_graph};

  std::vector<VertexId> view_vertices(std::from_range, graph_view.getVertices());
  std::vector<EdgeId> view_edges(std::from_range, graph_view.getEdges());
  EdgeEndpoints edge_endpoints = graph_view.getEdgeEndpoints(3);
  std::vector<VertexId> predecessors(std::from_range,
                                     graph_view.getPredecessors(3));
  std::vector<VertexId> successors(std::from_range,
                                   graph_view.getSuccessors(4));
  std::vector<OutgoingEdge> outgoing(std::from_range,
                                     graph_view.getOutgoingEdges(4));
  std::vector<IngoingEdge> ingoing(std::from_range,
                                   graph_view.getIngoingEdges(3));

  // Assert
  ASSERT_EQ(serialized_graph.size(), 64 + (128 + 128) * 2 + 128 + 64)
      << "Invalid byte size";
  std::byte *head = serialized_graph.data();
  for (int i = 0; i < serialized_graph.size(); i += 8, head += 8)
    std::cerr << "[" << i << "]: " << *(reinterpret_cast<std::uint64_t *>(head))
              << std::endl;

  EXPECT_EQ(view_vertices.size(), 9) << "Invalid number of vertices";
  EXPECT_EQ(view_edges.size(), 11) << "Invalid number of edges";
  EXPECT_EQ(edge_endpoints, edges[3]) << "Invalid edgepoints";
  EXPECT_TRUE(areSame(predecessors, {VertexId{1}, VertexId{2}}))
      << "Invalid predecessors: " << dump(predecessors);
  EXPECT_TRUE(areSame(successors, {VertexId{5}, VertexId{8}}))
      << "Invalid successors" << dump(successors);
  EXPECT_TRUE(areSame(outgoing, {OutgoingEdge{5, 5}, OutgoingEdge{8, 10}}))
      << "Invalid outoing edges" << dump(outgoing);
  EXPECT_TRUE(areSame(ingoing, {IngoingEdge{1, 2}, IngoingEdge{2, 3}}))
      << "Invalid ingoing edges " << dump(ingoing);
}

TEST(GraphTopology, NonEmptyGraph2) {
  // Arrange
  // Act
  // Assert
}

TEST(GraphTopology, SCCs) {
  // Arrange
  DirectedGraphTopology graph{};
  for (int i = 0; i < 9; i++)
    graph.addVertex();
  EdgeEndpoints edges[] = {
      EdgeEndpoints{0, 1}, EdgeEndpoints{1, 2}, EdgeEndpoints{1, 3},
      EdgeEndpoints{2, 3}, EdgeEndpoints{3, 4}, EdgeEndpoints{4, 5},
      EdgeEndpoints{5, 6}, EdgeEndpoints{6, 4}, EdgeEndpoints{6, 7},
      EdgeEndpoints{7, 6}, EdgeEndpoints{4, 8}};
  for (const auto &edge : edges)
    graph.addEdge(edge.from, edge.to);
  auto serialized_graph = serialize(graph);
  DirectedGraphTopologyView graph_view{serialized_graph};

  // Act
  auto scc_map = stronglyConnectedComponents(graph_view);

  for (const auto& e : scc_map){
    std::cout << e << " ";
  }
  std::cout << std::endl;
  // Assert
  std::array<int, 9> expected{1,2,9,3,4,4,4,4,5};
  for (int i=0;i<9;i++) EXPECT_EQ(scc_map[i], expected[i]);
}