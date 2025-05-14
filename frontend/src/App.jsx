import React, { useState, useEffect } from "react";
import "./App.css";
import SuccessPopup from "./SuccessPopup";

function App() {
  const [valg, setValg] = useState("");
  const [uid, setUid] = useState("");
  const [kortOK, setKortOK] = useState(false);
  const [showPopup, setShowPopup] = useState(false);
  const [status, setStatus] = useState("");
  const [fejl, setFejl] = useState("");
  const API_BASE = "http://localhost:5000";

  // Realtidsopdatering af UID og kortstatus
  useEffect(() => {
    const interval = setInterval(async () => {
      try {
        const res = await fetch(`${API_BASE}/seneste-uid`);
        if (!res.ok) throw new Error("Serverfejl");
        const data = await res.json();
        
        if (data.uid !== uid) {
          setUid(data.uid || "");
          setKortOK(data.valid);
        }
      } catch (err) {
        setFejl("Forbindelsesfejl - tjek backend");
        console.error(err);
      }
    }, 500);
    return () => clearInterval(interval);
  }, [uid]);

  // Autofjern fejlmeddelelser
  useEffect(() => {
    if (fejl) {
      const timer = setTimeout(() => setFejl(""), 5000);
      return () => clearTimeout(timer);
    }
  }, [fejl]);

  const confirmValg = async () => {
    if (!valg) {
      setFejl("Vælg en drik først!");
      return;
    }
    
    try {
      const res = await fetch(`${API_BASE}/gem-valg`, {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: valg,
      });
      
      if (!res.ok) throw new Error("Kunne ikke gemme valg");
      setShowPopup(true);
      setFejl("");
    } catch (err) {
      setFejl(err.message);
    }
  };

  const startBrygning = async () => {
    if (!kortOK || !uid) {
      setFejl("Ugyldigt kort eller manglende valg");
      return;
    }

    setStatus("Brygger din drik... ☕");
    
    try {
      const res = await fetch(`${API_BASE}/bestil`, { method: "POST" });
      const data = await res.json();
      
      if (data.status !== "Bestilling gennemført") throw new Error(data.error);
      
      setStatus("✅ Drik klar!");
      setTimeout(() => setStatus(""), 3000);
      resetState();
    } catch (err) {
      setFejl(err.message || "Brygningsfejl");
      setStatus("");
    }
  };

  const resetState = () => {
    setValg("");
    setUid("");
    setKortOK(false);
    setShowPopup(false);
  };

  return (
    <div className="App">
      <div className="container">
        <h1>☕ Kaffeautomat</h1>

        <div className="flow-section">
          <h2>1. Vælg drik</h2>
          <select
            value={valg}
            onChange={(e) => setValg(e.target.value)}
            disabled={!!status}
          >
            <option value="">-- Vælg --</option>
            <option value="Stor kaffe">Stor kaffe</option>
            <option value="Lille kaffe">Lille kaffe</option>
            <option value="Te">Te</option>
          </select>
          <button onClick={confirmValg} disabled={!!status}>
            Bekræft valg
          </button>
        </div>

        <div className="flow-section">
          <h2>2. Kortstatus</h2>
          <div className="uid-display">
            {uid ? (
              <>
                <code>{uid}</code>
                <span className={kortOK ? "status-ok" : "status-fejl"}>
                  {kortOK ? "Godkendt" : "Ikke godkendt"}
                </span>
              </>
            ) : (
              <span className="status-info">⌛ Vent på kort...</span>
            )}
          </div>
        </div>

        <div className="flow-section">
          <h2>3. Brygning</h2>
          <div className="button-group">
            <button 
              onClick={startBrygning} 
              disabled={!kortOK || !valg || !!status}
            >
              {status ? "Arbejder..." : "Start brygning"}
            </button>
            <button onClick={resetState} className="secondary">
              Nulstil
            </button>
          </div>
        </div>

        {showPopup && (
          <SuccessPopup
            message={`Valg gemt: ${valg}`}
            onClose={() => setShowPopup(false)}
          />
        )}

        {status && (
          <div className={`status-banner ${status.includes("✅") ? "success" : "info"}`}>
            {status}
          </div>
        )}

        {fejl && <div className="fejl-banner">{fejl}</div>}

        <div className="bestillingslog">
          <h3>Seneste bestillinger</h3>
          <BestillingsListe />
        </div>
      </div>
    </div>
  );
}

// Hjælpekomponent til bestillingsliste
const BestillingsListe = () => {
  const [bestillinger, setBestillinger] = useState([]);

  useEffect(() => {
    const hentBestillinger = async () => {
      try {
        const res = await fetch(`${API_BASE}/bestillinger`);
        const data = await res.json();
        setBestillinger(data.slice(-5).reverse());
      } catch (err) {
        console.error("Fejl ved hentning af bestillinger:", err);
      }
    };
    hentBestillinger();
    const interval = setInterval(hentBestillinger, 10000);
    return () => clearInterval(interval);
  }, []);

  return (
    <ul>
      {bestillinger.map((b, index) => (
        <li key={index}>
          <span className="drik">{b.valg}</span>
          <span className="tid">{b.timestamp}</span>
        </li>
      ))}
    </ul>
  );
};

export default App;
