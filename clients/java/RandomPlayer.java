import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.WebSocket;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.LinkedBlockingQueue;

public class RandomPlayer {
    private static class Listener implements WebSocket.Listener {
        private final BlockingQueue<String> messages;
        private final StringBuilder buffer = new StringBuilder();

        Listener(BlockingQueue<String> messages) {
            this.messages = messages;
        }

        @Override
        public CompletionStage<?> onText(WebSocket webSocket, CharSequence data, boolean last) {
            buffer.append(data);
            if (last) {
                messages.add(buffer.toString());
                buffer.setLength(0);
            }
            webSocket.request(1);
            return CompletableFuture.completedFuture(null);
        }
    }

    private static String extractString(String json, String key) {
        String pattern = "\"" + key + "\"";
        int pos = json.indexOf(pattern);
        if (pos < 0) return null;
        pos = json.indexOf(':', pos + pattern.length());
        if (pos < 0) return null;
        pos++;
        while (pos < json.length() && Character.isWhitespace(json.charAt(pos))) pos++;
        if (pos >= json.length() || json.charAt(pos) != '\"') return null;
        pos++;
        int end = json.indexOf('\"', pos);
        if (end < 0) return null;
        return json.substring(pos, end);
    }

    private static Integer extractInt(String json, String key) {
        String pattern = "\"" + key + "\"";
        int pos = json.indexOf(pattern);
        if (pos < 0) return null;
        pos = json.indexOf(':', pos + pattern.length());
        if (pos < 0) return null;
        pos++;
        while (pos < json.length() && Character.isWhitespace(json.charAt(pos))) pos++;
        int end = pos;
        while (end < json.length() && (Character.isDigit(json.charAt(end)) || json.charAt(end) == '-')) end++;
        if (end == pos) return null;
        return Integer.parseInt(json.substring(pos, end));
    }
    private static List<String> parseCsv(String value) {
        List<String> items = new ArrayList<>();
        if (value == null || value.trim().isEmpty()) return items;
        for (String token : value.split(",")) {
            String t = token.trim();
            if (!t.isEmpty()) items.add(t);
        }
        return items;
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.out.println("usage: java RandomPlayer ws://localhost:9002 room1 p1");
            return;
        }
        String uri = args[0];
        String roomId = args.length > 1 ? args[1] : "room1";
        String playerId = args.length > 2 ? args[2] : "p1";

        BlockingQueue<String> messages = new LinkedBlockingQueue<>();
        HttpClient client = HttpClient.newHttpClient();
        WebSocket webSocket = client.newWebSocketBuilder()
                .buildAsync(URI.create(uri), new Listener(messages))
                .join();

        System.out.println("Java client joining room=" + roomId + " player_id=" + playerId);
        String join = String.format("{\"type\":\"join\",\"room_id\":\"%s\",\"player_id\":\"%s\",\"role\":\"player\"}",
                roomId, playerId);
        webSocket.sendText(join, true).join();

        Random rng = new Random();
        while (true) {
            String msg = messages.take();
            String type = extractString(msg, "type");
            if (type == null) {
                continue;
            }
            if (type.equals("state")) {
                String legal = extractString(msg, "legal");
                List<String> choices = parseCsv(legal);
                if (!choices.isEmpty()) {
                    // Requesting discards reveals all players' discard information.
                    String discardsReq = String.format("{\"type\":\"discards_request\",\"room_id\":\"%s\"}", roomId);
                    webSocket.sendText(discardsReq, true).join();
                    String choice = choices.get(rng.nextInt(choices.size()));
                    String action = String.format("{\"type\":\"action\",\"room_id\":\"%s\",\"player_id\":\"%s\",\"choice\":\"%s\"}",
                            roomId, playerId, choice);
                    webSocket.sendText(action, true).join();
                }
            } else if (type.equals("discards")) {
                // System.out.println("discards " + msg);
            } else if (type.equals("game_over")) {
                Integer winner = extractInt(msg, "winner");
                System.out.println("game_over winner=" + winner);
                break;
            } else if (type.equals("error")) {
                String message = extractString(msg, "message");
                System.out.println("error: " + message);
            }
        }

        webSocket.sendClose(WebSocket.NORMAL_CLOSURE, "bye").join();
    }
}
