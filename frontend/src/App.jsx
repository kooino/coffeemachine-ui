// ✅ Komplet React frontend (App.js) tilpasset backend
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
    if (!valg) {
      setError("Vælg en drik først");
      return;
    }
    setError("");
    try {
      const res = await fetch("http://localhost:5000/gem-valg", {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: valg,
      });
      const data = await res.json();
      if (data.status === "Valg gemt") {
        setStatus("✅ Valg gemt");
      } else {
        setError("Kunne ikke gemme valg");
      }
    } catch (err) {
      console.error("Fejl ved gem-valg:", err);
    }
  };

  const sendBestil = async () => {
    if (!valid || !uid) {
      setError("Kort er ikke godkendt eller mangler");
      return;
    }
    try {
      const res = await fetch("http://localhost:5000/bestil", { method: "POST" });
      const data = await res.json();
      if (data.status === "Bestilling gennemført") {
        setStatus("☕ Din drik er klar!");
        setValg("");
      } else {
        setError(data.error || "Fejl ved bestilling");
      }
    } catch (err) {
      console.error("Fejl ved bestil:", err);
    }
  };

  return (
    <div style={{ padding: 30, fontFamily: "Arial" }}>
      <h1>☕ Kaffeautomat</h1>

      <h2>UID:</h2>
      <input
        type="text"
        value={uid}
        readOnly
        style={{ fontSize: "1.5em", width: "320px", marginBottom: 10 }}
      />
      <div style={{ marginBottom: 20 }}>
        {uid ? (valid ? "✅ Kort godkendt" : "❌ Kort ikke godkendt") : "⌛ Venter på kort..."}
      </div>

      <h2>Vælg din drik:</h2>
      <select value={valg} onChange={(e) => setValg(e.target.value)}>
        <option value="">-- Vælg --</option>
        <option value="Te">Te</option>
        <option value="Lille kaffe">Lille kaffe</option>
        <option value="Stor kaffe">Stor kaffe</option>
      </select>
      <br /><br />
      <button onClick={sendValg}>Bekræft valg</button>

      <h2 style={{ marginTop: 30 }}>Start brygning:</h2>
      <button onClick={sendBestil} disabled={!valid}>
        Start brygning
      </button>

      {status && (
        <div style={{ marginTop: 20, color: "green" }}>
          <strong>{status}</strong>
        </div>
      )}
      {error && (
        <div style={{ marginTop: 20, color: "red" }}>
          <strong>{error}</strong>
        </div>
      )}
    </div>
  );
}

export default App;
