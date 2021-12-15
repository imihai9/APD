import java.util.Collections;
import java.util.List;
import java.util.Map;

public class MapResult {
    private final String fileName;
    private final Map<Integer, Integer> dictionary; // dictionar lungime - numar aparitii
    private final List<String> maxWordList; // lista cu cuvintele de lungime maxima

    public MapResult (String fileName, Map<Integer, Integer> dictionary,
                      List<String> maxWordList) {
        this.fileName = fileName;
        this.dictionary = dictionary;
        this.maxWordList = maxWordList;
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
}
