package graph;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.*;


/**
 * Class designed to help in saving a graph to a file
 */
public class Writer {
	private Graph<Integer,Integer> graph;
	
	/**
	 * Empty class constructor
	 */
	public Writer()
	{
		
	}
	
	/**
	 * Function to assign the graph that you want to save to the Writer class
	 * @param graph Graph you wish to save
	 */
	public void setGraph(Graph<Integer,Integer> graph)
	{
		this.graph = graph;
	}
	
	
	
	/**
	 * Function that does the actual saving of the graph. 
	 * @param filename The name of the file that the graph will be saved to
	 * @throws IOException
	 */
	public void write(String filename) throws IOException
	{
		// try and write the file
		FileWriter writer = new FileWriter(filename);
		PrintWriter printer = new PrintWriter(writer);
		
		String nodeNumber = Integer.toString(graph.getNodes().size());
		printer.println(nodeNumber);
		
		// now we write all the edges
		List<Edge<Integer,Integer>> edges = new ArrayList<>(graph.getEdges());
		for(Edge<Integer,Integer> e: edges) 
		{		
			String line = e.getSource().toString() + " " + 
					      e.getTarget().toString() + " 1.0 LocalRoad" ;
			printer.println(line);
			
		}
		
		writer.close();
		
	}
	
	/**
	 * Static write function that allows you to save a graph without having
	 * to instantiate the Writer object. Saves a bit of coding
	 * @param filename The name of the file that the graph will be writen to
	 * @param g The graph that will be writen
	 * @throws IOException
	 */
	public static void write(String filename, Graph<Integer,Integer> g) throws IOException
	{
		Writer writer = new Writer();
		writer.setGraph(g);
		writer.write(filename);
	}
}
