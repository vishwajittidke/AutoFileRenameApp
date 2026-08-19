using System;
using System.Text.RegularExpressions;
using System.IO;

class Program {
    static void Main(string[] args) {
        foreach (string path in args) {
            string content = File.ReadAllText(path);
            Console.WriteLine($"File: {Path.GetFileName(path)}");
            Console.WriteLine($"Best Name: {GetBestName(content)}");
            Console.WriteLine();
        }
    }

    static string GetBestName(string content) {
        // 1. Specialized JSON logic
        if (content.TrimStart().StartsWith("{")) {
            var m = Regex.Match(content, @"""(?:name|title|project_id|id)""\s*:\s*""([^""]+)""", RegexOptions.IgnoreCase);
            if (m.Success) return m.Groups[1].Value;
        }

        // 2. Specialized Markdown logic
        var mdMatch = Regex.Match(content, @"^#\s+([^\r\n]+)", RegexOptions.Multiline);
        if (mdMatch.Success) return mdMatch.Groups[1].Value;

        // Frontmatter logic
        var fmMatch = Regex.Match(content, @"^(?:name|title)\s*:\s*([^\r\n]+)", RegexOptions.Multiline | RegexOptions.IgnoreCase);
        if (fmMatch.Success) return fmMatch.Groups[1].Value;

        // 3. Fallback generic logic (scan lines)
        string[] lines = content.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
        foreach (string rawLine in lines) {
            string line = rawLine.Trim();
            // Strip comments
            line = Regex.Replace(line, @"^(?:\/\/|#|\/\*|<!--|--|///|rem\s|'|\*)\s*", "", RegexOptions.IgnoreCase);
            line = line.Trim();
            
            // Skip lines that are just symbols (e.g. ------, ====)
            if (Regex.IsMatch(line, @"^[-=_*]+$")) continue;
            
            // Skip common programming constructs
            if (Regex.IsMatch(line, @"^(?:using|import|include|package|namespace|def|class|public|private)\b")) continue;
            if (Regex.IsMatch(line, @"^<[/a-zA-Z]")) continue; // XML/HTML tags
            if (Regex.IsMatch(line, @"^[{}[\]();,]+$")) continue; // Braces

            // Accept line if it has some alphanumeric characters
            if (Regex.IsMatch(line, @"[a-zA-Z]{3}")) {
                // If it starts with "File:", strip it
                if (line.StartsWith("File:", StringComparison.OrdinalIgnoreCase)) {
                    line = line.Substring(5).Trim();
                }
                
                // Sanitize: allow letters, numbers, spaces, dots, hyphens, underscores
                line = Regex.Replace(line, @"[^a-zA-Z0-9 _.-]", "");
                return line.Trim();
            }
        }
        return "TIMESTAMP_FALLBACK";
    }
}
