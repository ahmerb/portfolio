package player;

import scotlandyard.*;

import java.util.*;

/**
 * A class to store the state of a Scotland Yard game at some fixed point during
 * the game. An object of this type can be cloned.
 */
// TODO if playMove returns the cloned object, fields can be made final as the updated fields can be calculated before creating new instance of object.
public final class ScotlandYardState {

    private final ScotlandYardGraph graph;
    private final List<Colour> players;
    private Set<Colour> winningPlayers;
    private Map<Colour, Integer> playerLocations;
    private Map<Colour, Map<Ticket, Integer>> playerTickets;
    private boolean gameOver;
    private boolean ready;
    private Colour currentPlayer;
    private int round;
    private List<Boolean> rounds;


    private ScotlandYardState(ScotlandYardGraph graph, List<Colour> players, Map<Colour, Integer> playerLocations, Map<Colour, Map<Ticket, Integer>> playerTickets, Set<Colour> winningPlayers, boolean gameOver, boolean ready, Colour currentPlayer, int round, List<Boolean> rounds) {
        this.graph = graph;
        this.players = players;
        this.winningPlayers = new HashSet<>(winningPlayers);
        this.playerLocations = new HashMap<>(playerLocations);
        this.playerTickets = new HashMap<>(playerTickets);
        this.gameOver = gameOver;
        this.ready = ready;
        this.currentPlayer = currentPlayer;
        this.round = round;
        this.rounds = new ArrayList<>(rounds);
    }


    public ScotlandYardState(ScotlandYardView view, Integer
            location, ScotlandYardGraph graph) {
        this.graph = graph;
        this.players = view.getPlayers();

        this.winningPlayers = view.getWinningPlayers(); // NEW

        this.currentPlayer = view.getCurrentPlayer();

        this.playerLocations = new HashMap<>();
        for (Colour player : players) {
            if (player == currentPlayer)
                playerLocations.put(player, location);
            else
                playerLocations.put(player, view.getPlayerLocation(player));
        }

        this.playerTickets = new HashMap<>();
        for (Colour player : players) {
            HashMap<Ticket, Integer> thisPlayersTicketMap = new HashMap<>();
            for (Ticket ticket : Ticket.values()) {
                thisPlayersTicketMap.put(ticket, view.getPlayerTickets(player, ticket));
            }
            playerTickets.put(player, thisPlayersTicketMap);
        }

        this.gameOver = view.isGameOver();
        this.ready = view.isReady();
        this.round = view.getRound();
        this.rounds = view.getRounds();
    }


    /**
     * Returns a deep copy of this ScotlandYardState.
     *
     * @return a deep copy of this ScotlandYardState.
     */
    public ScotlandYardState copy() {
        return new ScotlandYardState(graph,
                                     players,
                                     playerLocations,
                                     playerTickets,
                                     winningPlayers,
                                     gameOver,
                                     ready,
                                     currentPlayer,
                                     round,
                                     rounds);
    }


    /**
     * Evaluates whether the next player to play is MrX.
     *
     * @return true is next player is MrX.
     */
    public boolean isNextPlayerMrX() {
        int currentIx = players.lastIndexOf(currentPlayer) + 1;
        int playersSize = players.size();
        return players.get(currentIx + 1 % playersSize).equals(Colour.Black);
    }

    /**
     * Returns the list of valid moves for a given player.
     *
     * @param player the player whose moves we want to see.
     * @return the list of valid moves for a given player.
     */
    public List<Move> validMoves(Colour player) {

        // store current location of player to get moves for
        int location = playerLocations.get(player);

        // generate all valid and invalid possible moves
        List<Move> moves = graph.generateMoves(player, location);

        // initialise empty validMoves list to store valid moves in
        List<Move> validMoves = new ArrayList<>();

        // Filter moves, and return valid moves:
        for (Move move : moves) {
            // if target not occupied and player has required tickets for
            // move, this move is valid
            boolean occupied = checkMoveTargetsNotOccupiedByADetective(move);
            if (!occupied && hasTickets(player, move))
                validMoves.add(move);
        }

        // if no valid moves, add a MovePass
        if (validMoves.isEmpty())
            validMoves.add(MovePass.instance(player));

        // all done.
        return validMoves;
    }

    // helpers for validMoves
    private boolean checkMoveTargetsNotOccupiedByADetective(Move move) {
        if (move instanceof MoveTicket)
            return checkMoveTargetsNotOccupiedByADetective((MoveTicket) move);
        else if (move instanceof MoveDouble)
            return checkMoveTargetsNotOccupiedByADetective((MoveDouble) move);
        else
            return true;
    }

    private boolean checkMoveTargetsNotOccupiedByADetective(MoveTicket move) {
        // store the move's target
        int dest = move.target;

        // if the target is already occupied, cannot move there, so
        // do not add to validMoves
        boolean occupied = false;
        // check the locations of all detectives
        for (Colour player : players.subList(1, players.size())) {
            if (playerLocations.get(player).intValue() == dest) {
                occupied = true;
            }
        }

        return occupied;
    }

    private boolean checkMoveTargetsNotOccupiedByADetective(MoveDouble move) {
        return checkMoveTargetsNotOccupiedByADetective(move.move1) &&
                checkMoveTargetsNotOccupiedByADetective(move.move2);
    }

    private boolean hasTickets(Colour player, Move move) {
        if (move instanceof MoveTicket) return hasTickets(player, (MoveTicket) move);
        else if (move instanceof MoveDouble) return hasTickets(player, (MoveDouble) move);
        else return true;
    }

    private boolean hasTickets(Colour player, MoveTicket move) {
        if (playerTickets.get(player).get(move.ticket) > 0) return true;
        else return false;
    }

    private boolean hasTickets(Colour player, MoveDouble move) {
        if (playerTickets.get(player).get(Ticket.Double) > 0) {
            MoveTicket move1 = move.move1;
            MoveTicket move2 = move.move2;
            if (move1.ticket.equals(move2.ticket)) {
                return playerTickets.get(player).get(move1.ticket) > 1;
            }
            else {
                return hasTickets(player, move1) && hasTickets(player, move2);
            }
        }
        return false;
    }


    /**
     * Plays a move sent from a player.
     *
     * @param move the move chosen by the player.
     */
    public void playMove(Move move) {
        play(move);
        nextPlayer();
        calculateIsGameOver();
    }


    // playMove helpers
    // TODO notify spectators?

    /**
     * Passes priority onto the next player whose turn it is to play.
     */
    protected void nextPlayer() {
        int nextPlayerIndex = (players.indexOf(currentPlayer) + 1) % players.size();
        currentPlayer = players.get(nextPlayerIndex);
    }

    /**
     * Allows the game to play a given move.
     *
     * @param move the move that is to be played.
     */
    protected void play(Move move) {
        if (move instanceof MoveTicket)
            play((MoveTicket) move);
        else if (move instanceof MoveDouble)
            play((MoveDouble) move);
        else if (move instanceof MovePass)
            play((MovePass) move);
    }

    /**
     * Plays a MoveTicket.
     *
     * @param move the MoveTicket to play.
     */
    private void play(MoveTicket move) {
        giveTicketsToMrX(move);
        updatePlayerTickets(move);
        updatePlayerLocation(move);
        updateRoundIfBlack();
        //notifySpectators();
    }

    /**
     * Plays a MoveDouble.
     *
     * @param move the MoveDouble to play.
     */
    private void play(MoveDouble move) {
        //notifySpectators(move);
        updatePlayerTickets(move);
        play(move.move1);
        play(move.move2);
    }

    /**
     * Plays a MovePass.
     *
     * @param move the MovePass to play.
     */
    private void play(MovePass move) {
        //notifySpectators(move);
    }

    /**
     * Increments current round if it is mrX's turn
     */
    private void updateRoundIfBlack() {
        if (currentPlayer.equals(Colour.Black))
            round++;
    }

    /**
     * Gives the ticket(s) a detective uses when playing a move to mrX
     *
     * @param move the move to determine what tickets to give to mrX
     */
    private void giveTicketsToMrX(Move move) {
        // if current player detective, update corresponding tickets in
        // playersMap used in for tickets used in move for mrX
        if (move.colour != Colour.Black) {
            if (move instanceof MoveTicket)
                playerTickets.get(Colour.Black).computeIfPresent(((MoveTicket) move).ticket, (k, v) -> v + 1);
        }
    }

    /**
     * Removes the corresponding tickets for a player when a move is played.
     *
     * @param move the move that specifies the tickets to remove
     */
    private void updatePlayerTickets(Move move) {
        // decrement player's tickets in playerTickets map
        if (move instanceof MoveTicket) {
            playerTickets.get(move.colour).computeIfPresent(((MoveTicket) move).ticket, (k, v) -> v - 1);
        }
        else if (move instanceof MoveDouble) {
            playerTickets.get(move.colour).computeIfPresent(Ticket.Double, (k, v) -> v - 1);
            // tickets for specific moves in double move are removed when
            // play(MoveTicket move) is called in play(MoveDouble move)
        }
    }

    /**
     * Updates a players location when a move is played.
     *
     * @param move the move that specifies the location to move to
     */
    private void updatePlayerLocation(MoveTicket move) {
        playerLocations.put(move.colour, move.target);
    }


    // Calculate if game is over

    /**
     * Returns the colours of the winning players. If Mr X it should contain a single
     * colour, else it should send the list of detective colours
     *
     * @return A set containing the colours of the winning players
     */
    public Set<Colour> calculateWinningPlayers() {
        calculateIsGameOver(); // updates winningPlayers too
        return winningPlayers;
    }


    /**
     * The game is over when MrX has been found or the agents are out of
     * tickets. See the rules for other conditions.
     *
     * @return true when the game is over, false otherwise.
     */
    public boolean calculateIsGameOver() {
        // case: game not yet ready
        if (!isReady())
            return false;

        // case: MrX has won
        if (areAllDetectivesStuck() || areAllTurnsPlayed() || isOnlyOnePlayer()) {
            winningPlayers = new HashSet<>();
            winningPlayers.addAll(Collections.singletonList(Colour.Black));
            return true;
        }

        // case: Detectives have won
        Set<Colour> detectiveColours = new HashSet<>(players);
        detectiveColours.remove(Colour.Black);
        if (isMrXCaught() || isMrXStuck()) {
            winningPlayers = new HashSet<>();
            winningPlayers.addAll(detectiveColours);
            return true;
        }

        // case: otherwise game not over
        return false;
    }


    /**
     * @return true if all detectives in game are unable to make a move
     */
    private boolean areAllDetectivesStuck() {
        List<Colour> detectives = players.subList(1, players.size());

        for (Colour detective : detectives) {
            if (!isDetectiveStuck(detective))
                return false;
        }
        return true;
    }


    /**
     * Determines if a detective is stuck. A detective is stuck if it has no
     * tickets for the available transport links from their current location.
     *
     * @param detective the detective PlayerData to check if stuck or not
     * @return true if detective is stuck
     */
    private boolean isDetectiveStuck(Colour detective) {
        return validMoves(detective).contains(MovePass.instance(detective));
    }


    /**
     * @return true if MrX is unable to make a move
     */
    private boolean isMrXStuck() {
        return validMoves(Colour.Black).isEmpty();
    }


    /**
     * @return true if number of rounds has been played and detectives have
     * played their turns
     */
    private boolean areAllTurnsPlayed() {
        return currentPlayer == Colour.Black && (round >= rounds.size() - 1);
    }


    /**
     * @return true if MrX is caught by a detective
     */
    private boolean isMrXCaught() {
        List<Colour> detectives = players.subList(1, players.size());

        for (Colour detective : detectives) {
            if (Objects.equals(playerLocations.get(detective), playerLocations.get(Colour.Black)))
                return true;
        }
        return false;
    }


    /**
     * @return true if there is currently only one player in the game
     */
    private boolean isOnlyOnePlayer() {
        return (players.size() == 1);
    }


    // GETTERS

    public Map<Colour, Integer> getPlayerLocations() {
        return playerLocations;
    }

    public Map<Colour, Map<Ticket, Integer>> getPlayerTickets() {
        return playerTickets;
    }

    public Set<Colour> getWinningPlayers() {
        return winningPlayers;
    }

    public boolean isGameOver() {
        return gameOver;
    }

    public boolean isReady() {
        return ready;
    }

    public Colour getCurrentPlayer() {
        return currentPlayer;
    }

    public int getRound() {
        return round;
    }

    public List<Boolean> getRounds() {
        return rounds;
    }

    public ScotlandYardGraph getGraph() {
        return graph;
    }

    public List<Colour> getPlayers() {
        return players;
    }

}
