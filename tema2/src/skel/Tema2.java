package skel;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class Tema2 {
    int workersCnt;
    String inFileName;
    String outFileName;

    int fragmentSize;
    int documentsCnt;
    String[] documentNames;

    void readData() {
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

    void splitInput() {
        //InputSplit + Launch map threads
        ExecutorService tpe = Executors.newFixedThreadPool(workersCnt);
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
                tpe.submit(new Task(documentNames[i], fragmentSize, offset));
            }

            // Last fragment can have a shorter size
            int offset = (fragmentsCnt - 1) * fragmentSize;
            tpe.submit(new Task(documentNames[i], lastFragmentSize, offset));
        }

        // Ends executor service when all submitted tasks have finished running (~join).
        tpe.shutdown();
    }

    public static void main(String[] args) {
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
    }
}
