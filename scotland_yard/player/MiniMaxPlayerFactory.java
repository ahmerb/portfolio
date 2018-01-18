package player;

import net.PlayerFactory;
import scotlandyard.Colour;
import scotlandyard.Player;
import scotlandyard.ScotlandYardView;
import scotlandyard.Spectator;

import java.util.ArrayList;
import java.util.List;

/**
 *
 */
public class MiniMaxPlayerFactory implements PlayerFactory {

    @Override
    public Player getPlayer(Colour colour, ScotlandYardView view, String mapFilename) {
        //TODO: Update this with your AI implementation.
        return new MiniMaxPlayer(view, mapFilename, colour);
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
