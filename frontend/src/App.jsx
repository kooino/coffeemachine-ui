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

  useEffect(() => {
    const interval = setInterval(async () => {
      try {
        const res = await fetch(`${API_BASE}/seneste-uid`);
        const data = await res.json();
        if (data.uid && data.uid !== uid) {
          setUid(data.uid);
        }
      } catch (err) {
        console.error("Fejl ved hentning af UID:", err);
      }
    }, 2000);
    return () => clearInterval(interval);
  }, [uid]);

  const confirmValg = async () => {
    if (!valg) {
      setFejl("Vælg en drik først!");
      return;
    }
    setFejl("");
    try {
      const res = await fetch(`${API_BASE}/gem-valg`, {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: valg,
      });

      if (!res.ok) throw new Error("HTTP-fejl ved valg");

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

  const scanKort = async () => {
    if (!uid) {
      setFejl("Indtast UID først!");
      return;
    }
    setFejl("");
    try {
      const res = await fetch(`${API_BASE}/tjek-kort?uid=${encodeURIComponent(uid)}`);
      if (!res.ok) throw new Error("HTTP-fejl ved kortscan");

      const data = await res.json();
      setKortOK(data.kortOK);

      if (!data.kortOK) {
        setFejl("Kort ugyldigt!");
      }
    } catch (error) {
      console.error(error);
      setFejl("Fejl ved scanning af kort!");
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

      if (!res.ok) throw new Error("HTTP-fejl ved bestilling");

      const data = await res.json();

      if (data.status === "OK") {
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
      setKortOK(false);
      setUid("");
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
        <input
          type="text"
          placeholder="Indtast UID (f.eks. 165267797)"
          value={uid}
          onChange={(e) => setUid(e.target.value)}
        />
        <br />
        <button onClick={scanKort}>Scan kort</button>

        <h3>Status:</h3>
        <p>
          {kortOK
            ? "✅ Kort godkendt!"
            : uid
            ? "❌ Kort ikke godkendt endnu!"
            : "Indtast UID og scan kort"}
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
