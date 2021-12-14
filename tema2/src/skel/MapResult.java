import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public class MapResult {
    private final String fileName;
    private final Map<Integer, Integer> dictionary; // dictionar lungime - numar aparitii
    private final List<String> maxWordList; // lista cu cuvintele de lungime maxima

    public MapResult (String fileName) {
        this.fileName = fileName;
        this.dictionary = new HashMap<>();
        this.maxWordList = new LinkedList<>();
    }

    public String getFileName() {
        return fileName;
    }

    public List<String> getMaxWordList() {
        return Collections.unmodifiableList(maxWordList);
    }

    public Map<Integer, Integer> getDictionary() {
        return Collections.unmodifiableMap(dictionary);
    }

    public void addToDict(Integer key, Integer value) {
        dictionary.put(key, value);
    }

    public void addToMaxWordList(String word) {
        maxWordList.add(word);
    }

    public void clearWordList() {
        maxWordList.clear();
    }

    @Override
    public String toString() {
        return "MapResult{" +
                "\ndictionary=" + dictionary +
                "\nmaxWordList=" + maxWordList +
                "}\n";
    }
}
