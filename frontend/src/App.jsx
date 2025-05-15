import React, { useState, useEffect } from "react";

function App() {
  const [uid, setUid] = useState("");
  const [valid, setValid] = useState(false);
  const [valg, setValg] = useState("");
  const [status, setStatus] = useState("");
  const [error, setError] = useState("");

  useEffect(() => {
    const interval = setInterval(async () => {
      try {
        const res = await fetch("http://localhost:5000/seneste-uid");
        const data = await res.json();
        setUid(data.uid || "");
        setValid(data.valid || false);
      } catch (err) {
        console.error("Fejl ved hentning af UID:", err);
      }
    }, 1000);
    return () => clearInterval(interval);
  }, []);

  const sendValg = async () => {
    if (!valg) return setError("Vælg en drik først");
    setError("");
    try {
      const res = await fetch("http://localhost:5000/gem-valg", {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: valg,
      });
      const data = await res.json();
      if (data.status === "Valg gemt") setStatus("✅ Valg gemt");
    } catch (err) {
      setError("Kunne ikke sende valg");
    }
  };

  const sendBestil = async () => {
    if (!valid || !uid) return setError("Kort ikke godkendt");
    try {
      const res = await fetch("http://localhost:5000/bestil", { method: "POST" });
      const data = await res.json();
      if (data.status === "Bestilling gennemført") {
        setStatus("☕ Din drik er på vej!");
        setValg("");
      } else {
        setError(data.error || "Ukendt fejl");
      }
    } catch (err) {
      setError("Fejl ved bestilling");
    }
  };

  return (
    <div style={{ padding: 30, fontFamily: "Arial", maxWidth: 400 }}>
      <h1>☕ Kaffeautomat</h1>
      <p><strong>UID:</strong> {uid || "⌛ Venter på kort..."}</p>
      <p>{uid ? (valid ? "✅ Godkendt kort" : "❌ Afvist kort") : ""}</p>

      <h3>Vælg drik:</h3>
      <select value={valg} onChange={(e) => setValg(e.target.value)}>
        <option value="">-- vælg --</option>
        <option value="Te">Te</option>
        <option value="Lille kaffe">Lille kaffe</option>
        <option value="Stor kaffe">Stor kaffe</option>
      </select>
      <br /><br />
      <button onClick={sendValg}>Gem valg</button>

      <h3 style={{ marginTop: 20 }}>Start brygning:</h3>
      <button onClick={sendBestil} disabled={!valid}>Start</button>

      {status && <p style={{ color: "green" }}>{status}</p>}
      {error && <p style={{ color: "red" }}>{error}</p>}
    </div>
  );
}

export default App;
