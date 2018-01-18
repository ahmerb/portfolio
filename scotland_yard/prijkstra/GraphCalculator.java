package prijkstra;

// COMS10001/COMS10004
// (code compacted for screen presentation)

import java.util.*;
import graph.*;

// implements base algorithm for Prim's and Dijkstra's
public abstract class GraphCalculator<X, Y> {

  // reference to input graph
  private Graph<X, Y> graph;

  // constructor
  public GraphCalculator(Graph<X, Y> graph) {
	this.graph = graph;	  
  }
	
  // applies algorithm to input graph
  public Graph<X, Y> getResult(X startNodeID, Weighter<Y> edgeWeighter) {
	
	// define node collections
	final Set<Node<X>> visited = new HashSet<>();
	final PriorityQueue<Node<X>> unvisited = new PriorityQueue<>();
	
	// make a result graph
	Graph<X, Y> ourResult = new DirectedGraph<>();
	
	// initialise starting node
	Node<X> currentNode = graph.getNode(startNodeID);
	currentNode.setWeight(0.0);
	visited.add(currentNode);
	
	// initialise node collections
	for (Node<X> node : graph.getNodes()) {
	  ourResult.add(node);
	  if (!currentNode.getIndex().equals(node.getIndex())) {
	    node.setWeight(Double.POSITIVE_INFINITY);
	    unvisited.add(node);
      }
    }
	
	// find initial direct distances to start node
	updateDistances(unvisited, currentNode, ourResult, edgeWeighter);
	  
	// greedily update nodes
	while (!unvisited.isEmpty()) {
	  visited.add(currentNode);
	  currentNode = unvisited.poll();
	  updateDistances(unvisited, currentNode, ourResult, edgeWeighter);
	}
	
	// return result
	return ourResult;
  }
  
  // update rule to be specified
  protected abstract Double update(Double distance, Double currentDistance, Double directDistance );

  // updates all unvisited node distances by considering routes via currentNode
  private void updateDistances(PriorityQueue<Node<X>> unvisited,
	 	                       Node<X> currentNode,
		                       Graph<X, Y> ourResult,
                               Weighter<Y> edgeWeighter) {
	
	// consider neighbours of current node (others can't gain from update)
    for(Edge<X, Y> e : graph.getEdgesFrom(currentNode)) {
      Node<X> neighbour = e.getTarget();
      
      // only update unvisited nodes (others already have shortest distance)
      if (unvisited.contains(neighbour)) {
   	    Double distance = neighbour.getWeight();

   	    // apply update rule (here with fixed edge weight of 1.0)
        Double possibleUpdate = update(distance, currentNode.getWeight(), edgeWeighter.toWeight(e.getData()));

        // only update nodes with better option
        if (distance > possibleUpdate) {
          unvisited.remove(neighbour);
          neighbour.setWeight(possibleUpdate);
          unvisited.add(neighbour);

          // add reverse edge to result (to create paths to start node rather than from)
          ourResult.add(new Edge<>(neighbour, currentNode, e.getData()));
        }
      }
    }
  }

}