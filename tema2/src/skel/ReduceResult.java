public class ReduceResult {
    private final String fileName;
    private final float rank;
    private final int maxLength;
    private final int countMaxLengthWords;

    public ReduceResult(String fileName, float rank, int maxLength, int countMaxLengthWords) {
        this.fileName = fileName;
        this.rank = rank;
        this.maxLength = maxLength;
        this.countMaxLengthWords = countMaxLengthWords;
    }

    public String getFileName() {
        return fileName;
    }

    public float getRank() {
        return rank;
    }

    public int getMaxLength() {
        return maxLength;
    }

    public int getCountMaxLengthWords() {
        return countMaxLengthWords;
    }

    @Override
    public String toString() {
        StringBuilder strb = new StringBuilder();

        strb.append(fileName);
        String rankFormatted = String.format("%.2f", rank);
        strb.append(',');
        strb.append(rankFormatted); // rank format
        strb.append(',').append(maxLength).append(',').append(countMaxLengthWords);
        return strb.toString();
    }
}
