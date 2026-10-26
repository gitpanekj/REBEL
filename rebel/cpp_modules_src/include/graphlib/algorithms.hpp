/**
 * @file algorithms.hpp
 * @author Jan Pánek (xpanek11@stud.fit.vutbr.cz)
 * @brief Graph algorithms.
 * @version 0.1
 * @date 2026-10-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef GRAPHLIB_ALGORITHMS
#define GRAPHLIB_ALGORITHMS

#include "graphlib/graph_types.hpp"
#include <vector>
#include <stack>
#include <span>


namespace graphlib {

using LowLinkValue = VertexId;
using DiscoveryTime = VertexId;
using SCCId = VertexId;
template <GraphType T>
void __scc_search(const T &g, VertexId vertex, std::span<LowLinkValue> ll,
                  std::span<DiscoveryTime> dt, std::span<SCCId> cid,
                  std::span<bool> on_stack, VertexId &time) {
  std::stack<VertexId> scc_stack;
  std::stack<std::tuple<VertexId, VertexId, bool>>
      dfs_stack; // predecessor, vertex

  dfs_stack.push({vertex, vertex, false});
  while (!dfs_stack.empty()) {
    auto &[predecessor, curr_vertex, processed] = dfs_stack.top();

    if (processed) { // try to determine SCCid

      ll[predecessor] = std::min(ll[predecessor], ll[curr_vertex]);

      dfs_stack.pop();
      if (ll[curr_vertex] == dt[curr_vertex]) {
        VertexId top;
        while ((top = scc_stack.top()) != curr_vertex) {
          on_stack[top] = false;
          cid[top] = ll[curr_vertex];
          scc_stack.pop();
        }
        // pop the root of an scc
        on_stack[top] = false;
        cid[top] = ll[curr_vertex];
        scc_stack.pop();
      }
    } else { // conitnue with DFS to successors
      processed = true;
      dt[curr_vertex] = time;
      ll[curr_vertex] = time;
      time++;
      on_stack[curr_vertex] = true;
      scc_stack.push(curr_vertex);

      for (const auto successor : g.getSuccessors(curr_vertex)) {
        if (dt[successor] == 0) { // not discovered yet by DFS
          dfs_stack.push(std::tuple{curr_vertex, successor, false});
          // low link update done after the succesor is processed - if
          // (processed)
        } else if (on_stack[successor]) {
          ll[curr_vertex] = std::min(ll[curr_vertex], dt[successor]);
        }
      }
    }
  }
}

template <GraphType T>
[[nodiscard]] std::vector<SCCId>
stronglyConnectedComponents(const T &obj) noexcept {
  VertexId time = 1;

  auto n_vertices = obj.getVertexNumber();
  constexpr std::size_t elem_size = sizeof(LowLinkValue) +
                                    sizeof(DiscoveryTime) + sizeof(SCCId) +
                                    sizeof(bool);
  std::vector<std::byte> buffer(n_vertices * elem_size, std::byte{0});
  std::span<LowLinkValue> ll(reinterpret_cast<LowLinkValue *>(buffer.data()),
                             n_vertices);
  std::span<DiscoveryTime> dt(
      reinterpret_cast<LowLinkValue *>(buffer.data() +
                                       n_vertices * sizeof(LowLinkValue)),
      n_vertices);
  std::span<SCCId> cid(
      reinterpret_cast<LowLinkValue *>(
          buffer.data() +
          n_vertices * (sizeof(LowLinkValue) + sizeof(DiscoveryTime))),
      n_vertices);
  std::span<bool> on_stack(
      reinterpret_cast<bool *>(
          buffer.data() + n_vertices * (sizeof(LowLinkValue) +
                                        sizeof(DiscoveryTime) + sizeof(SCCId))),
      n_vertices);

  for (const auto vertex : obj.getVertices()) {
    if (cid[vertex] == 0) { // unassinged SCC id
      __scc_search(obj, vertex, ll, dt, cid, on_stack, time);
    }
  }

  return std::vector<SCCId>(std::begin(cid), std::end(cid));
}
}; // namespace graphlib

#endif