// Offline verification tool using an already installed beatoraja decoder.
// Arguments: private manifest TSV, output directory. No audio/assets are opened.
import bms.model.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.*;

public class ExtractBmsPatterns {
    public static void main(String[] args) throws Exception {
        Path output = Path.of(args[1]);
        Files.createDirectories(output.resolve("notes"));
        int accepted = 0, skipped = 0;
        try (BufferedWriter manifest = Files.newBufferedWriter(output.resolve("decoded.tsv"))) {
            for (String line : Files.readAllLines(Path.of(args[0]), StandardCharsets.UTF_8)) {
                String[] fields = line.split("\t", 5);
                int profile = Integer.parseInt(fields[1]);
                try {
                    BMSDecoder decoder = new BMSDecoder(0);
                    BMSModel model = decoder.decode(Path.of(fields[4]));
                    if (model == null || model.containsMineNote() ||
                        (profile == 5 && model.getMode() != Mode.BEAT_5K) ||
                        (profile == 8 && model.getMode() != Mode.BEAT_7K)) { skipped++; continue; }
                    boolean bad = false;
                    for (DecodeLog log : decoder.getDecodeLog())
                        if (log.getState().toString().equals("ERROR")) bad = true;
                    StringBuilder events = new StringBuilder();
                    for (TimeLine time : model.getAllTimeLines()) {
                        for (int lane = 0; lane < model.getMode().key; lane++) {
                            Note note = time.getNote(lane);
                            if (note == null) continue;
                            if (profile == 5 && model.getMode().isScratchKey(lane)) { bad = true; continue; }
                            if (note instanceof LongNote && ((LongNote)note).isEnd()) continue;
                            long end = note instanceof LongNote ? ((LongNote)note).getPair().getMilliTime() : time.getMilliTime();
                            if (note instanceof LongNote && ((LongNote)note).getType() > LongNote.TYPE_LONGNOTE) bad = true;
                            events.append(lane).append(' ').append(time.getMilliTime()).append(' ').append(end).append('\n');
                        }
                    }
                    if (bad || events.length() == 0) { skipped++; continue; }
                    Files.writeString(output.resolve("notes").resolve(fields[0]+".txt"), events);
                    manifest.write(String.join("\t", fields[0], fields[1], fields[2], fields[3])+"\n");
                    if (++accepted % 100 == 0) System.out.println("decoded="+accepted);
                } catch (Exception e) { skipped++; }
            }
        }
        System.out.println("accepted="+accepted+" skipped="+skipped);
    }
}
