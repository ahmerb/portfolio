package player;

import net.*;
import scotlandyard.*;

import java.io.IOException;
import java.util.*;

/**
 * The RandomPlayerFactory is an example of a PlayerFactory that
 * gives the AI server your AI implementation. You can also put any
 * code that you want to run before and after a game in the methods
 * provided here.
 */
public class RandomPlayerFactory implements PlayerFactory {

    @Override
    public Player getPlayer(Colour colour, ScotlandYardView view, String mapFilename) {
        //TODO: Update this with your AI implementation.
        return new RandomPlayer(view, mapFilename);
    }

    @Override
    public void ready() {
        //TODO: Any code you need to execute when the game starts, put here.
    }

    @Override
    public List<Spectator> getSpectators(ScotlandYardView view) {
        List<Spectator> spectators = new ArrayList<>();
        //TODO: Add your AI here if you want it to be a spectator.
        return spectators;
    }

    @Override
    public void finish() {
        //TODO: Any code you need to execute when the game ends, put here.
    }

}
