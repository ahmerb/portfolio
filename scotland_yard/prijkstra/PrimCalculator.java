package prijkstra;

// COMS10001/COMS10004
// (code compacted for screen presentation)

import graph.*;

// implements Prim's
public class PrimCalculator extends GraphCalculator {
	
  // constructor
  public PrimCalculator(Graph<Integer,Integer> graph) {
	super(graph);
  }
  
  //implements Prim's update rule
  protected Double update(Double distance, Double currentDistance, Double directDistance ) {
    return Math.min(distance, directDistance);
} }