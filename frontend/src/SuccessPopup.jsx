import React, { useEffect, useState } from "react";
import "./SuccessPopup.css";

const SuccessPopup = ({ message, onClose }) => {
  const [fadeOut, setFadeOut] = useState(false);

  useEffect(() => {
    // Tid før fade starter (i ms)
    const FADE_DELAY = 2000;
    // Tid før komponent fjernes helt
    const REMOVE_DELAY = 2800;

    const timer1 = setTimeout(() => setFadeOut(true), FADE_DELAY);
    const timer2 = setTimeout(() => {
      if (typeof onClose === "function") {
        onClose();
      }
    }, REMOVE_DELAY);

    return () => {
      clearTimeout(timer1);
      clearTimeout(timer2);
    };
  }, [onClose]);

  return (
    <div className={`popup ${fadeOut ? "fade-out" : ""}`}>
      <span>{message}</span>
    </div>
  );
};

export default SuccessPopup;
