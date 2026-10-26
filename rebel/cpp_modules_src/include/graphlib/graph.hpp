#ifndef GRAPH_H
#define GRAPH_H

#include "graphlib/graph_labels.hpp"
#include "graphlib/graph_topology.hpp"
#include "graphlib/graph_types.hpp"

namespace graphlib {

template <Serializable LabelT>
class Graph : public DirectedGraphTopology, public LabelStorage<LabelT> {
public:
  Graph() = default;
  Graph(DirectedGraphTopologyView &topo, LabelStorageView &labels)
      : DirectedGraphTopology(topo), LabelStorage<LabelT>(labels) {}
};

} // namespace graphlib

#endif