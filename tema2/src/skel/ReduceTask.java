import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.RecursiveTask;
import java.util.stream.Collectors;

public class ReduceTask extends RecursiveTask<Void> {
    // Input
    private final List<MapResult> mapResultList;

    // Output
    private ReduceResult result;

    private List<String> finalMaxWordList;
    private final Map<Integer, Integer> finalDictionary;
    private int maxLength;
    private float rank;

    // Other data
    private int[] fiboSeq;
    private String fileName;

    public ReduceTask(List<MapResult> mapResultList) {
        this.mapResultList = mapResultList;
        this.finalMaxWordList = new LinkedList<>();
        this.finalDictionary = new HashMap<>();
        this.rank = 0.0f;

        this.fileName = mapResultList.get(0).getFileName();
        int lastSlashOccur = fileName.lastIndexOf('/');
        if (lastSlashOccur != -1)
            fileName = fileName.substring(lastSlashOccur + 1);
    }

    // For easier sorting in main
    public float getRank() {
        return rank;
    }

    public ReduceResult getResult() {
        return this.result;
    }

    private void combineStep() {
        for (MapResult partialRes : mapResultList) {
            // Merge dictionaries
            partialRes.getDictionary().forEach(
                    (key, value) -> finalDictionary.merge(key, value, Integer::sum)
            );
        }

        // Get max word length
        maxLength = finalDictionary.keySet().stream().max(Integer::compareTo).orElse(0);

        // Get combined list of max length words
        finalMaxWordList = mapResultList.stream()
                .flatMap(s -> s.getMaxWordList().stream())
                .filter(x -> x.length() == maxLength)
                .collect(Collectors.toList());

        //TODO: maybe make this more efficient? (if time permits)
    }

    private void generateFibonacci (int k) {
        fiboSeq = new int[k + 1];
        fiboSeq[0] = fiboSeq[1] = 1;
        for (int i = 2; i <= k; i++) {
            fiboSeq[i] = fiboSeq[i - 1] + fiboSeq[i - 2];
        }
    }

    private void reduceStep() {
        // Compute rank
        float rankNumerator = 0.0f;
        generateFibonacci(maxLength + 1);

        long totalWordCount = 0;

        for (Map.Entry<Integer, Integer> dictEntry : finalDictionary.entrySet()) {
            int wordLength = dictEntry.getKey();
            int wordCount = dictEntry.getValue();
            rankNumerator += fiboSeq[wordLength] * wordCount;

            totalWordCount += wordCount;
        }

        this.rank = (float) rankNumerator / totalWordCount;
    }

    @Override
    public Void compute() {
        combineStep();
        reduceStep();

        result = new ReduceResult(
                fileName,
                rank,
                maxLength,
                finalMaxWordList.size());

        return null;
    }
}
