package skel;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

public class Task implements Runnable {
    // Key: fileName (?)
    private final String fileName;
    private final int fragSize;
    private final int fragOffset;
    // Value: ce generez aici

    // Other data
    private final StringBuilder fragmentBuilder;
    private String fragment;
    private int partialBufferSize = 10; // Size of buffer used to read incomplete word
    public static final String separators = ";:/? ̃\\.,><‘[]{}()!@#$%ˆ&- +’=*”| \t\n\r";

    public Task (String fileName, int fragSize, int fragOffset) {
        this.fileName = fileName;
        this.fragSize = fragSize;
        this.fragOffset = fragOffset;
        System.out.println(fileName + " " + fragSize + " " + fragOffset);
        this.fragmentBuilder = new StringBuilder();
    }

    private void adjustFragmentStart(char[] buffer, BufferedReader reader) {
        // If fragment ends with an incomplete word => read half fragment until whitespace reached.
        if (separators.indexOf(buffer[fragSize]) == -1 &&
                separators.indexOf(buffer[fragSize + 1]) == -1) {

            fragmentBuilder.append(buffer, 0, fragSize + 2);

            boolean foundEOW = false; // found end of word
            int index, charsRead = 0;
            do {
                try {
                    charsRead = reader.read(buffer, 0, partialBufferSize);
                } catch (IOException | NullPointerException e) {
                    e.printStackTrace();
                }

                if (charsRead < partialBufferSize) {
                    partialBufferSize = charsRead;
                    foundEOW = true;
                }

                index = 0;
                while (index < partialBufferSize &&
                        separators.indexOf(buffer[index]) == -1) {
                    index++;
                }
                // copy buffer (partial word) to fragment
                fragmentBuilder.append(buffer, 0, index);

                if (index < partialBufferSize)
                    foundEOW = true;

            } while (!foundEOW);

        } else {
            fragmentBuilder.append(buffer, 0, fragSize + 1);
        }
    }

    private void adjustFragmentEnd(BufferedReader reader) {
        // If fragment starts with an incomplete word => skip that word
        int i = 0;
        while (i < fragmentBuilder.length() && separators.indexOf(fragmentBuilder.charAt(i)) == -1)
            i++;

        fragment = fragmentBuilder.substring(i);
    }

    private void readFragment() {
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(fileName)); //, fragSize + 2)
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }

        char[] buffer = new char[fragSize + 2]; // assigns fragment + 2 extra chars (start, end)
        buffer[0] = ' '; // placeholder for further checks
        try {
            assert reader != null;
            if (fragOffset > 0) {
                reader.skip(fragOffset - 1);
                reader.read(buffer, 0, fragSize + 2);
            }
            else {
                reader.skip(fragOffset);
                reader.read(buffer, 1, fragSize + 1);
            }
        } catch (IOException | NullPointerException e) {
            e.printStackTrace();
        }

        adjustFragmentStart(buffer, reader);
        adjustFragmentEnd(reader);

        try {
            reader.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        System.out.println("!!!!!OFFSET " + fragOffset + " " + fragment);

        //TODO: tokenize
    }

    @Override
    public void run() {
        readFragment();
    }
}
