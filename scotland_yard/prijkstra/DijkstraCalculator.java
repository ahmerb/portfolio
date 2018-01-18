package prijkstra;

// COMS10001/COMS10004
// (code compacted for screen presentation)

import graph.*;

// implements Dijkstra's
public class DijkstraCalculator<X, Y> extends GraphCalculator<X, Y> {

  public DijkstraCalculator(Graph<X, Y> graph) {
	super(graph);
  }
  
  // implements Dijkstra's update rule
  protected Double update(Double distance, Double currentDistance, Double directDistance ) {
	  return Math.min(distance, currentDistance + directDistance);
  }
  
  // runs Dijkstra's algorithm and output particular route
  public Graph<X, Y> getResult(X startNodeID, X destinationNodeID, Weighter<Y> edgeWeighter) {
    
	// calculate graph with paths from every node to start node with its distance
	Graph<X, Y> startToAnyNode = getResult(startNodeID, edgeWeighter);

    // trace route from end node to start node
    Node<X> current = startToAnyNode.getNode(destinationNodeID);
    Graph<X, Y> route = new DirectedGraph<>();
    route.add(current);
    while (!startToAnyNode.getEdgesFrom(current).isEmpty()) {
      Edge<X, Y> e = startToAnyNode.getEdgesFrom(current).get(0);
      route.add(e.getTarget());
      route.add(e);
      current = e.getTarget();
    }
    return route;
  }

}