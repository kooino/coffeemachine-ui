// src/App.js
import React, { useState, useEffect } from "react";
import "./App.css";
import SuccessPopup from "./SuccessPopup";

function App() {
  const [valg, setValg] = useState("");
  const [uid, setUid] = useState("");
  const [kortOK, setKortOK] = useState(false);
  const [showPopup, setShowPopup] = useState(false);
  const [brygger, setBrygger] = useState(false);
  const [status, setStatus] = useState("");
  const [fejl, setFejl] = useState("");

  const API_BASE = "http://localhost:5000";

  // Hent automatisk UID og kortstatus hvert sekund
  useEffect(() => {
    const interval = setInterval(async () => {
      try {
        const res = await fetch(`${API_BASE}/seneste-uid`);
        if (!res.ok) throw new Error("Fejl ved hentning");
        const data = await res.json();
        setUid(data.uid || "");
        setKortOK(data.valid || false);
      } catch (error) {
        console.error("Fejl ved UID-fetch:", error);
      }
    }, 1000);
    return () => clearInterval(interval);
  }, []);

  const confirmValg = async () => {
    if (!valg) return setFejl("Vælg en drik først!");
    setFejl("");
    try {
      const res = await fetch(`${API_BASE}/gem-valg`, {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: valg,
      });
      const data = await res.json();
      if (data.status === "Valg gemt") {
        setShowPopup(true);
      } else {
        setFejl("Kunne ikke gemme valg.");
      }
    } catch (error) {
      console.error(error);
      setFejl("Fejl ved valg. Prøv igen!");
    }
  };

  const startBrygning = async () => {
    if (!kortOK) {
      setFejl("Kort ikke godkendt!");
      return;
    }

    setFejl("");
    setBrygger(true);
    setStatus("Brygger din drik ...");

    try {
      const res = await fetch(`${API_BASE}/bestil`, {
        method: "POST",
      });

      const data = await res.json();

      if (data.status === "Bestilling gennemført") {
        setStatus("☕ Din drik er klar! Tag din kop.");
        setTimeout(() => setStatus(""), 4000);
      } else if (data.error) {
        setFejl(data.error);
        setStatus("");
      }
    } catch (error) {
      console.error(error);
      setFejl("Fejl ved brygning!");
      setStatus("");
    } finally {
      setValg("");
      setBrygger(false);
    }
  };

  const aflysBestilling = () => {
    setValg("");
    setKortOK(false);
    setUid("");
    setShowPopup(false);
    setBrygger(false);
    setStatus("");
    setFejl("");
  };

  return (
    <div className="App">
      <div className="container">
        <h1>☕ Kaffeautomat ☕</h1>

        <h2>1. Vælg drik</h2>
        <select value={valg} onChange={(e) => setValg(e.target.value)}>
          <option value="">-- Vælg --</option>
          <option value="Stor kaffe">Stor kaffe</option>
          <option value="Lille kaffe">Lille kaffe</option>
          <option value="Te">Te</option>
        </select>
        <br />
        <button onClick={confirmValg}>Bekræft valg</button>

        {showPopup && (
          <SuccessPopup
            message={`✅ Valg gemt: ${valg}`}
            onClose={() => setShowPopup(false)}
          />
        )}

        <h2>2. Scan kort</h2>
        <p><strong>UID:</strong> {uid || "⌛ Venter på kort..."}</p>
        <p>
          {uid
            ? (kortOK ? "✅ Kort godkendt!" : "❌ Kort ikke godkendt endnu!")
            : "Læg kort på læser..."}
        </p>

        <h2>3. Start brygning</h2>
        <button onClick={startBrygning} disabled={!kortOK || brygger}>
          Start brygning
        </button>
        <button onClick={aflysBestilling} className="cancel-btn">
          Afbryd
        </button>

        {status && (
          <div className="status-box">
            <strong>{status}</strong>
          </div>
        )}

        {fejl && (
          <div className="error-box">
            <strong>{fejl}</strong>
          </div>
        )}
      </div>
    </div>
  );
}

export default App;
