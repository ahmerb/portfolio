package prijkstra;

/**
 * A helper interface for Prijkstra. A Weighter<T> object should assign a weight
 * to an object of type T when toWeight is called. In Prijkstra's, a Weighter is
 * used to get the weight of an edge, so can calculate the distance between
 * nodes.
 */
public interface Weighter<T> {
    /**
     * Takes an object of type T and assigns and returns a weight for it.
     *
     * @param t the object to get weight for.
     * @return the weight of the object.
     */
    double toWeight(T t);
}
