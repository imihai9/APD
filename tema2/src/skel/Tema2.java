import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Scanner;
import java.util.concurrent.ForkJoinPool;

public class Tema2 {
    private int workersCnt;
    private String inFileName;
    private String outFileName;

    private int fragmentSize;
    private int documentsCnt;
    private String[] documentNames;

    private List<MapTask> mapTaskList;
    private List<ReduceTask> reduceTaskList;

    public Tema2 () {
        this.mapTaskList = new LinkedList<>();
        this.reduceTaskList = new LinkedList<>();
    }

    private void readData() {
        // inFile parsing
        File inFile = new File(inFileName);
        Scanner in = null;

        try {
            in = new Scanner(inFile);
        } catch (FileNotFoundException e) {
            System.out.println("File not found!");
            e.printStackTrace();
        }

        assert in != null;
        fragmentSize = in.nextInt();
        documentsCnt = in.nextInt();
        documentNames = new String[documentsCnt];

        for (int i = 0; i < documentsCnt; i++) {
            documentNames[i] = in.next();
        }

        in.close();
    }

    private void splitInput() {
        //InputSplit
        for (int i = 0; i < documentsCnt; i++) {
            File file = new File(documentNames[i]);
            long fileLen = file.length();

            int fragmentsCnt;
            int lastFragmentSize = (int) fileLen % fragmentSize;
            if (lastFragmentSize > 0)
                fragmentsCnt = (int) fileLen / fragmentSize + 1;
            else
                fragmentsCnt = (int) fileLen / fragmentSize;

            for (int frag = 0; frag < fragmentsCnt - 1; frag++) {
                int offset = frag * fragmentSize;
                mapTaskList.add(new MapTask(documentNames[i], fragmentSize, offset));
            }

            // Last fragment can have a shorter size
            int offset = (fragmentsCnt - 1) * fragmentSize;
            mapTaskList.add(new MapTask(documentNames[i], lastFragmentSize, offset));
        }
    }

    private void combine() {
        // TODO: do this in reduce task, with stream for the corresponding doc name
        // TODO OR: do this in map task, with thread-safe hashmap
        // Maps document name -> partial map operation results (MapResult)
        Map<String, List<MapResult>> docMapResults = new HashMap<>();
        // Initialize map with all document names and a corresponding empty list
        for (int i = 0; i < documentsCnt; i++)
            docMapResults.put(documentNames[i], new LinkedList<>());

        for (MapTask mapTask : mapTaskList) {
            MapResult mapResult = mapTask.getResult();
            if (!mapResult.getDictionary().isEmpty()) {
                List<MapResult> docList = docMapResults.get(mapResult.getFileName());
                docList.add(mapResult);
            }
        }

        // Create tasks for each document, having the list of partial map results as arg.
        for (Map.Entry<String, List<MapResult>> docMapResult : docMapResults.entrySet()) {
            reduceTaskList.add(new ReduceTask(docMapResult.getValue()));
        }
    }

    private void launchMappers() {
        ForkJoinPool fjp = new ForkJoinPool(workersCnt);
        for (MapTask mapTask : mapTaskList) {
            fjp.invoke(mapTask);
        }
        // Wait for all tasks to finish, then shutdown executor (~join)
        fjp.shutdown();
    }

    private void launchReducers() {
        ForkJoinPool fjp = new ForkJoinPool(workersCnt);
        for (ReduceTask reduceTask : reduceTaskList) {
            fjp.invoke(reduceTask);
        }
        // Wait for all tasks to finish, then shutdown executor (~join)
        fjp.shutdown();
    }

    private void writeOutput() throws IOException {
        BufferedWriter writer = new BufferedWriter(new FileWriter(outFileName));

        //TODO: make reduceTaskList and mapTaskList arrays, to sort more efficiently.
        reduceTaskList.sort(Comparator.comparing(ReduceTask::getRank).reversed());
        for (ReduceTask reduceTask : reduceTaskList) {
           writer.write(reduceTask.getResult().toString() + "\n");
        }

        writer.close();
    }

    public static void main(String[] args) throws IOException {
        Tema2 tema = new Tema2();

        if (args.length < 3) {
            System.err.println("Usage: skel.Tema2 <workers> <in_file> <out_file>");
            return;
        }

        tema.workersCnt = Integer.parseInt(args[0]);
        tema.inFileName = args[1];
        tema.outFileName = args[2];

        tema.readData();

        tema.splitInput();
        tema.launchMappers();
        tema.combine();
        tema.launchReducers();

        tema.writeOutput();
    }
}
