// Offline Revive reference extraction with an independently installed beatoraja.
// Only chart bytes are read; original charts, keysounds and images stay untouched.
import bms.model.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.*;

public class ExtractRevivePatterns {
    static String sha256(byte[] raw) throws Exception {
        return HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(raw));
    }

    public static void main(String[] args) throws Exception {
        Path output = Path.of(args[1]);
        Map<String, String> decoded = new LinkedHashMap<>();
        List<String> audit = new ArrayList<>();
        List<String> diagnostics = new ArrayList<>();
        audit.add("sha256\tnotes\tholds\tcharge_holds\tlnmode\tdecoder_mode\tdecoder_messages");
        for (String line : Files.readAllLines(Path.of(args[0]), StandardCharsets.UTF_8)) {
            String[] fields = line.split("\t", 2);
            if (fields.length != 2 || !fields[0].matches("[0-9a-f]{64}"))
                throw new IllegalArgumentException("Invalid stage manifest");
            byte[] raw = Files.readAllBytes(Path.of(fields[1]));
            if (!sha256(raw).equals(fields[0])) throw new IllegalArgumentException("Chart hash mismatch");
            // Refuse branch-dependent charts: a random seed is not a course identity.
            String ascii = new String(raw, StandardCharsets.ISO_8859_1);
            if (java.util.regex.Pattern.compile("(?im)^#(?:RANDOM|SETRANDOM|IF)\\b").matcher(ascii).find())
                throw new IllegalArgumentException("Conditional chart is not supported");
            BMSDecoder decoder = new BMSDecoder(0);
            BMSModel model = decoder.decode(raw, false, null);
            if (model == null || model.getMode() != Mode.BEAT_10K || model.containsMineNote())
                throw new IllegalArgumentException("Not a supported 10K chart: " + fields[0]);
            for (DecodeLog log : decoder.getDecodeLog()) {
                diagnostics.add(fields[0] + "\t" + log.getState() + "\t" + log.getMessage().replace('\n', ' ').replace('\r', ' ').replace('\t', ' '));
                // Several authentic Revive files say PLAYER 1 despite containing
                // both hands. The decoder's channel-derived mode is authoritative.
                // All other warnings can hide a dropped/malformed note and fail closed.
                boolean playerWarning = log.getMessage().equals("#PLAYER定義が1にもかかわらず2P側のノーツ定義が存在します");
                if (log.getState().toString().equals("ERROR") ||
                    (log.getState().toString().equals("WARNING") && !playerWarning))
                    throw new IllegalArgumentException("BMS decoding error: " + fields[0] + ": " + log.getMessage());
            }
            StringBuilder events = new StringBuilder();
            int notes = 0, holds = 0, charges = 0;
            for (TimeLine time : model.getAllTimeLines()) {
                int mapped = 0;
                for (int lane = 0; lane < model.getMode().key; lane++) {
                    Note note = time.getNote(lane);
                    if (model.getMode().isScratchKey(lane)) {
                        if (note != null) throw new IllegalArgumentException("Playable scratch in 10K source");
                        continue;
                    }
                    int target = mapped++;
                    if (note == null) continue;
                    long start = time.getMilliTime(), end = start;
                    if (note instanceof LongNote) {
                        LongNote hold = (LongNote)note;
                        if (hold.isEnd()) continue;
                        if (hold.getPair() == null || hold.getType() > LongNote.TYPE_CHARGENOTE)
                            throw new IllegalArgumentException("Unsupported or unpaired hold");
                        end = hold.getPair().getMilliTime();
                        if (end <= start) throw new IllegalArgumentException("Invalid hold duration");
                        // CN release judgement is not simulated. The estimator consumes
                        // physical down/hold/up burden, shared by normal LN and CN.
                        holds++;
                        if (hold.getType() == LongNote.TYPE_CHARGENOTE) charges++;
                    }
                    if (start < 0 || target >= 10) throw new IllegalArgumentException("Invalid event");
                    events.append(target).append(' ').append(start).append(' ').append(end).append('\n');
                    notes++;
                }
                if (mapped != 10) throw new IllegalArgumentException("Unexpected decoder lane layout");
            }
            if (notes == 0 || decoded.putIfAbsent(fields[0], events.toString()) != null)
                throw new IllegalArgumentException("Empty or duplicate source chart");
            audit.add(fields[0] + "\t" + notes + "\t" + holds + "\t" + charges + "\t" + model.getLnmode() + "\t" + model.getMode() + "\t" + decoder.getDecodeLog().length);
        }
        // Fail before writing outputs if any required chart was invalid.
        Files.createDirectories(output.resolve("stages"));
        for (var item : decoded.entrySet())
            Files.writeString(output.resolve("stages").resolve(item.getKey() + ".txt"), item.getValue(), StandardCharsets.UTF_8);
        Files.write(output.resolve("decode-audit.tsv"), audit, StandardCharsets.UTF_8);
        Files.write(output.resolve("decode-diagnostics.tsv"), diagnostics, StandardCharsets.UTF_8);
        System.out.println("decoded_stages=" + decoded.size());
    }
}
