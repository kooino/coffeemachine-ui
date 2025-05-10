import React, { useEffect, useState } from "react";
import "./SuccessPopup.css";

const SuccessPopup = ({ message, onClose }) => {
  const [fadeOut, setFadeOut] = useState(false);

  useEffect(() => {
    const timer1 = setTimeout(() => setFadeOut(true), 2000);     // start fade
    const timer2 = setTimeout(onClose, 2800);                    // remove from DOM

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
