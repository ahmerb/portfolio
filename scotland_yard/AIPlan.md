# Plan:

## Tree improvements
    - Make max score every 6 turns, not every other.
    - Add alpha-beta pruning.
    - Add quiescence search.


## calculateScore() improvements
   - Reduce score when near corner of map. DONE (based on degree of node)
   - Make MrX choose double move and secret when having to show location.
   - Don't let dijkstra include Boat edges when calculating shortest route for a
     detective to get to MrX. DONE (massively increased Boat weight)
   - Reduce score massively if MrX loses. DONE
   - Make MiniMaxPlayer a spectator, and use transport used to keep track of
     possible locations for each player.