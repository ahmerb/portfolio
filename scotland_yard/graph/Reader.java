package graph;

import java.util.*;
import java.io.*;


/**
 * Class to help read a graph from a file
 */
public class Reader
{
    private Graph<Integer,Integer> graph;

    /**
     * Function to obtain the loaded graph
     * @return The loaded graph. Will be null if the graph 
     * has not been loaded
     */
    public Graph<Integer,Integer> graph()
    {
    	return graph;
    }
    

    /**
     * Function that does the actual reading of the graph.
     * @param filename The filename of the file that contains the graph
     * @throws IOException
     */
    public void read(String filename) throws IOException
    {
    	
    	// initialise the graph
    	graph = new UndirectedGraph<>();
    	
    	// load the file
        File file = new File(filename);
        Scanner in = new Scanner(file);
        
        // get the top line
        String topLine = in.nextLine();
        
        int numberOfNodes = Integer.parseInt(topLine);

        
        // create the number of nodes
        for(int i = 0; i < numberOfNodes; i++)
        {
        	Node<Integer> n = new Node<>(i + 1);
        	graph.add(n);
        }
        
        
        while (in.hasNextLine())
        {
            String line = in.nextLine();
            
            String[] names = line.split(" ");
            String id1 = names[0];
            String id2 = names[1];
            int mtype;
            
            if(names[3].equals("LocalRoad"))
            {
            	mtype = 0;
            }
            else if(names[3].equals("Underground"))
            {
            	mtype = 1;
            }
            else 
            {
            	mtype = 2;
            }
            
            // create the edge
            
            Edge<Integer,Integer> edge = new Edge<>(graph.getNode(Integer.parseInt(id1)),
                                                           graph.getNode(Integer.parseInt(id2)), new Integer(mtype));
            if (mtype==0) {
            	graph.add(edge);
            }
                 
        }
        
        in.close();
    }    
}