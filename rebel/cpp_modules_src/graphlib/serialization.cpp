
#include "graphlib/graph_topology.hpp"
#include "graphlib/graph_types.hpp"
#include <cstring>
#include <vector>

namespace graphlib {
std::vector<std::byte> serialize(const DirectedGraphTopology &obj) {

  auto no_vertices = obj.getVertexNumber();
  auto no_edges = obj.getEdgeNumber();
  //  Sizes of individual items padded to multiple of 64 bytes (cache line size)
  offset_t header_size = paddToMultipleOf(
      sizeof(DirectedGraphTopologyView::GraphTopologyViewHeader), 64);
  offset_t outgoing_edge_offset_list_size =
      paddToMultipleOf((no_vertices + 1) * sizeof(offset_t),
                       64); // no_vertices+1 for CSR sentinel
  offset_t outgoing_edge_list_size =
      paddToMultipleOf(no_edges * sizeof(OutgoingEdge), 64);
  offset_t ingoing_edge_offset_list_size =
      paddToMultipleOf((no_vertices + 1) * sizeof(offset_t),
                       64); // no_vertices+1 for CSR sentinel
  offset_t ingoing_edge_list_size =
      paddToMultipleOf(no_edges * sizeof(IngoingEdge), 64);
  offset_t edge_endpoints_size =
      paddToMultipleOf(no_edges * sizeof(EdgeEndpoints), 64);
  // padding to multiples of 512 bits
  offset_t visibility_mask_size = paddToMultipleOf((no_edges + 7) / 8, 64);

  offset_t total_size = header_size + outgoing_edge_offset_list_size +
                        outgoing_edge_list_size +
                        ingoing_edge_offset_list_size + ingoing_edge_list_size +
                        edge_endpoints_size + visibility_mask_size;

  // Compute offset list and flatten adjacency lists
  std::vector<offset_t> outgoing_offsets((no_vertices + 1), 0);
  std::vector<OutgoingEdge> flattened_outgoing_edges;
  flattened_outgoing_edges.reserve(no_edges);
  for (std::size_t i = 0; i < no_vertices; i++) {
    auto outgoing_edges = obj.getOutgoingEdges(i);
    outgoing_offsets[i + 1] = outgoing_offsets[i] + outgoing_edges.size();
    flattened_outgoing_edges.append_range(outgoing_edges);
  }

  std::vector<offset_t> ingoing_offsets((no_vertices + 1), 0);
  std::vector<IngoingEdge> flattened_ingoing_edges;
  flattened_ingoing_edges.reserve(no_edges);
  for (std::size_t i = 0; i < no_vertices; i++) {
    auto ingoing_edges = obj.getIngoingEdges(i);
    ingoing_offsets[i + 1] = ingoing_offsets[i] + ingoing_edges.size();
    flattened_ingoing_edges.append_range(ingoing_edges);
  }

  // Padd visibility mask
  std::vector<uint64_t> padded_visibility_mask(visibility_mask_size, 0);
  for (const auto e : obj.getEdges()) {
    if (obj.isEdgeVisible(e)) {
      padded_visibility_mask[e / 64] |= (1ULL << (e % 64));
    }
  }

  // Edge Endpoints
  std::vector<EdgeEndpoints> edge_endpoints(edge_endpoints_size);
  for (const auto e : obj.getEdges()) {
    edge_endpoints[e] = obj.getEdgeEndpoints(e);
  }

  // Populate buffer and init header
  std::vector<std::byte> buffer(total_size);
  std::byte *base_ptr = buffer.data();

  DirectedGraphTopologyView::GraphTopologyViewHeader header{};
  offset_t current_offset = 0;

  auto append_to_buffer = [&](offset_t &offset_field, const void *data,
                              offset_t raw_bytes, offset_t padded_bytes) {
    offset_field = current_offset;
    if (raw_bytes > 0 && data != nullptr) {
      std::memcpy(base_ptr + current_offset, data, raw_bytes);
    }
    current_offset += padded_bytes;
  };

  header.n_vertices = no_vertices;
  header.n_edges = no_edges;
  current_offset += header_size;

  append_to_buffer(
      header.outgoing_edge_offset_list_offset, outgoing_offsets.data(),
      (no_vertices + 1) * sizeof(offset_t), outgoing_edge_offset_list_size);
  append_to_buffer(header.outgoing_edge_list_offset,
                   flattened_outgoing_edges.data(),
                   no_edges * sizeof(OutgoingEdge), outgoing_edge_list_size);
  append_to_buffer(header.ingoing_edge_offset_list_offset,
                   ingoing_offsets.data(), (no_vertices + 1) * sizeof(offset_t),
                   ingoing_edge_offset_list_size);
  append_to_buffer(header.ingoing_edge_list_offset,
                   flattened_ingoing_edges.data(),
                   no_edges * sizeof(IngoingEdge), ingoing_edge_list_size);
  append_to_buffer(header.edge_endpoints_offset, edge_endpoints.data(),
                   no_edges * sizeof(EdgeEndpoints), edge_endpoints_size);
  append_to_buffer(header.visibility_mask_offset, padded_visibility_mask.data(),
                   visibility_mask_size, visibility_mask_size);
  std::memcpy(base_ptr, &header,
              sizeof(DirectedGraphTopologyView::GraphTopologyViewHeader));

  return buffer;
}
} // namespace graphlib