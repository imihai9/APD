import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.StringTokenizer;
import java.util.concurrent.RecursiveTask;

import static java.lang.Math.min;

public class MapTask extends RecursiveTask<Void> {
    // Key: fileName (?)
    private final String fileName;
    private final int fragSize;
    private final int fragOffset;

    // Value: ce generez aici
    private MapResult result;

    // Other data
    private final StringBuilder fragmentBuilder;
    private String fragment;
    private int partialBufferSize; // Size of buffer used to read an incomplete word
    public static final String separators = ";:/?~\\.,><`[]{}()!@#$%^&-_+'=*\"| \t\r\n\r\0";
    private int maxWordLength;

    public MapTask(String fileName, int fragSize, int fragOffset) {
        this.fileName = fileName;
        this.fragSize = fragSize;
        this.fragOffset = fragOffset;

        this.result = new MapResult(fileName);
        this.maxWordLength = 0;

        this.fragmentBuilder = new StringBuilder();
        this.partialBufferSize = min(fragSize, 10);
    }

    public MapResult getResult() {
        return result;
    }

    private void adjustFragmentStart(BufferedReader reader) {
        // If fragment starts with an incomplete word => skip that word
        int i = 0;
        while (i < fragmentBuilder.length() && separators.indexOf(fragmentBuilder.charAt(i)) == -1)
            i++;
        // Get rid of beginning whitespaces
        while (i < fragmentBuilder.length() && separators.indexOf(fragmentBuilder.charAt(i)) != -1)
            i++;
        fragment = fragmentBuilder.substring(i);
    }

    private void adjustFragmentEnd(char[] buffer, BufferedReader reader) {
        if (!(separators.indexOf(buffer[fragSize]) == -1 &&
                separators.indexOf(buffer[fragSize + 1]) == -1))
            return;

        // If fragment ends with an incomplete word => read half fragment until whitespace reached.
        // Append the extra last character
        fragmentBuilder.append(buffer, fragSize + 1, 1);

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

            if (index != 0) {
                // copy buffer (partial word) to fragment
                fragmentBuilder.append(buffer, 0, index);
            }

            if (index < partialBufferSize)
                foundEOW = true;

        } while (!foundEOW);
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

        int readSkip, readOffset, readLen, charsRead = 0; // reader variables

        if (fragOffset > 0) {
            readSkip = fragOffset - 1;
            readOffset = 0;
            readLen = fragSize + 2;
        }
        else {
            readSkip = fragOffset;
            readOffset = 1;
            readLen = fragSize + 1;
        }

        try {
            assert reader != null;
            reader.skip(readSkip);
            charsRead = reader.read(buffer, readOffset, readLen);

        } catch (IOException | NullPointerException e) {
            e.printStackTrace();
        }

        // Copy the fragment + extra start character
        fragmentBuilder.append(buffer, 0, fragSize + 1);

        if (charsRead == readLen)   // if an extra char at end could be read
            adjustFragmentEnd(buffer, reader);
        adjustFragmentStart(reader);

        try {
            reader.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void tokenize() {
        StringTokenizer itr = new StringTokenizer(fragment, separators);
        //TODO: compute first here, then move variables to Result
        while (itr.hasMoreTokens()) {
            String token = itr.nextToken();

            int wordLength = token.length();

            if (wordLength == maxWordLength)
                result.addToMaxWordList(token);
            else if (wordLength > maxWordLength) {
                maxWordLength = wordLength;
                if (wordLength > 0)
                    result.clearWordList();
                result.addToMaxWordList(token);
            }

            int count = result.getDictionary().getOrDefault(wordLength, 0);
            result.addToDict(wordLength, count + 1);
        }
    }

    @Override
    protected Void compute() {
        readFragment();
        if (fragment.isEmpty()) {
            return null;
        }

//        if (fileName.equals("tests/files/sonnets_10")) {
//            System.out.println("OFFSET " + fragOffset);
//            System.out.println(fragment);
//        }

        tokenize();
        return null;
    }
}
