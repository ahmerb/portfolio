package aigraph;

import graph.DirectedGraph;
import graph.Edge;
import player.ScotlandYardState;
import scotlandyard.Move;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.PriorityQueue;

/**
 *
 */
public class ScotlandYardGameTree extends DirectedGraph<ScotlandYardState, Move> {

    private final AINode head;

    /**
     * Creates a game tree with just the parent node. Node's score is set to a
     * default of zero.
     *
     * @param currentGameState ScotlandYardView implementing object to be root
     *                         node of ScotlandYardGameTree.
     */
    public ScotlandYardGameTree(ScotlandYardState currentGameState) {
        super();
        this.head = new AINode(currentGameState, 0.0);
        this.add(head);
    }


    /**
     * Adds an edge to the graph. Multiple edges with the same source and
     * target nodes are allowed.
     *
     * @param edge the edge to add to the graph.
     */
    @Override
    public void add(Edge<ScotlandYardState, Move> edge) {
        super.add(edge);
    }


    /**
     * Returns a list with the edges from head to it's direct child nodes.
     *
     * @return a list of edges from head node to it's children.
     */
    public List<Edge<ScotlandYardState, Move>> getListFirstLevelEdges() {
        List<Edge<ScotlandYardState, Move>> firstLevelEdges = new ArrayList<>();

        // for each edge from head node, add that edge to the return list
        for (Edge<ScotlandYardState, Move> e : getEdgesFrom(head)) {
            firstLevelEdges.add(e);
        }

        return firstLevelEdges;
    }


    /**
     * Returns a list with the scores associated with each direct child of the
     * head node.
     *
     * @return a list with the scores associated with each direct child of the
     *         head node.
     */
    public List<Double> getFirstLevelScores() {

        List<Double> firstLevelScores = new ArrayList<>();

        for (Edge<ScotlandYardState, Move> e : getListFirstLevelEdges()) {
            firstLevelScores.add(((AINode) e.getTarget()).getScore());
        }

        return firstLevelScores;
    }


    /**
     * Gets the head of the tree.
     *
     * @return the AINode which is the head of the tree.
     */
    public AINode getHead() {
        return head;
    }


    /**
     * Returns children of node in a tree as a List.
     *
     * @param node the node to get the children of.
     * @return a list with the children of node.
     */
    public List<AINode> getChildren(AINode node) {
        List<AINode> children = new ArrayList<>();
        for (Edge<ScotlandYardState, Move> e : this.getEdgesFrom(node)) {
            children.add((AINode) e.getTarget());
        }
        return children;
    }


    /**
     * Returns the children of node in a tree as a PriorityQueue.
     * Must pass in comparator to give AINode's an ordering.
     *
     * @param node the node to get the children of.
     * @param comp the comparator of AINode's to determine their order.
     * @return a PriorityQueue with the children of node.
     */
    public PriorityQueue<AINode> getChildrenQueue(AINode node,
                                                  Comparator<AINode> comp) {
        PriorityQueue<AINode> children = new PriorityQueue<>(comp);
        for (Edge<ScotlandYardState, Move> e : this.getEdgesFrom(node)) {
            children.add((AINode) e.getTarget());
        }
        return children;
    }

    /**
     * Returns the parent node of an AINode in a tree.
     *
     * @param node the node to find the parent for.
     * @return the parent node.
     * @throws RuntimeException if for whatever reason node does not have a
     *                          single parent.
     */
    public AINode getParent(AINode node) {
        if (!getNodes().contains(node)) throw new RuntimeException("node not " +
                "in tree");
        if (getEdgesTo(node).size() == 0) throw new RuntimeException("node " +
                "has no parent");
        if (node.equals(head)) throw new RuntimeException("node is head so" +
                "has no parent");
        if (getEdgesTo(node).size() != 1) throw new RuntimeException("more " +
                "than one edge to node");

        return (AINode) getEdgesTo(node).get(0).getSource();
    }

}
