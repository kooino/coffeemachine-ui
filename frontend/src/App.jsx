import React, { useState, useEffect } from "react";
import "./App.css";
import SuccessPopup from "./SuccessPopup";

function App() {
  const [valg, setValg] = useState("");
  const [kortOK, setKortOK] = useState(false);
  const [showPopup, setShowPopup] = useState(false);
  const [brygger, setBrygger] = useState(false);
  const [status, setStatus] = useState("");
  const [fejl, setFejl] = useState("");
  const [aflyser, setAflyser] = useState(false);

  const API_BASE = "http://localhost:5000";

  useEffect(() => {
    const interval = setInterval(async () => {
      try {
        const res = await fetch(`${API_BASE}/tjek-kort`);
        const data = await res.json();
        setKortOK(data.kortOK || false);
        setFejl(data.error || "");
      } catch (err) {
        console.error("Fejl ved kortstatus:", err);
        setFejl("Kan ikke hente kortstatus");
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
        setTimeout(() => setShowPopup(false), 3000);
      } else {
        setFejl(data.error || "Kunne ikke gemme valg.");
      }
    } catch (error) {
      console.error(error);
      setFejl("Fejl ved valg.");
    }
  };

  const startBrygning = async () => {
    if (!kortOK) return setFejl("Kort ikke godkendt!");
    setFejl("");
    setBrygger(true);
    setStatus("Brygger din drik ...");

    try {
      const res = await fetch(`${API_BASE}/bestil`, { method: "POST" });
      const data = await res.json();
      if (data.status === "OK") {
        setStatus("☕ Din drik er klar! Tag din kop.");
        setTimeout(() => setStatus(""), 4000);
      } else {
        setFejl(data.error);
        setStatus("");
      }
    } catch (error) {
      console.error(error);
      setFejl("Fejl under brygning!");
      setStatus("");
    } finally {
      setValg("");
      setBrygger(false);
    }
  };

  const aflysBestilling = async () => {
    if (aflyser) return;
    setAflyser(true);

    try {
      const res = await fetch(`${API_BASE}/annuller`, { method: "POST" });
      const data = await res.json();
      if (data.status === "Annulleret") {
        setStatus("Bestilling annulleret.");
        setTimeout(() => setStatus(""), 3000);
      }
    } catch (err) {
      console.error("Fejl ved annullering:", err);
    }

    setValg("");
    setKortOK(false);
    setShowPopup(false);
    setBrygger(false);
    setStatus("");
    setFejl("");
    setAflyser(false);
  };

  return (
    <div className="App">
      <div className="header">
        <img src="/kaffekop.png" alt="Kaffeautomat Logo" className="logo" />
        <h1>☕ Velkommen til Kaffeautomaten</h1>
        <p>Scan dit kort og vælg en drik</p>
      </div>

      <div className="container">
        <h2>1. Vælg drik</h2>
        <select value={valg} onChange={(e) => setValg(e.target.value)}>
          <option value="">-- Vælg --</option>
          <option value="Stor kaffe">Stor kaffe</option>
          <option value="Lille kaffe">Lille kaffe</option>
          <option value="Te">Te</option>
        </select>
        <button onClick={confirmValg}>Bekræft valg</button>

        {showPopup && <SuccessPopup message={`✅ Valg gemt: ${valg}`} />}

        <h2>2. Scan kort</h2>
        <p>{kortOK ? "✅ Kort godkendt!" : "⌛ Venter på kort..."}</p>

        <h2>3. Start brygning</h2>
        <button onClick={startBrygning} disabled={!kortOK || brygger}>
          Start brygning
        </button>
        <button onClick={aflysBestilling} className="cancel-btn" disabled={aflyser}>
          Afbryd
        </button>

        {status && <div className="status-box"><strong>{status}</strong></div>}
        {fejl && <div className="error-box"><strong>{fejl}</strong></div>}
      </div>
    </div>
  );
}

export default App;
