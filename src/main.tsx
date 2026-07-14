
  import { createRoot } from "react-dom/client";
  import App from "./app/App.tsx";
  import "./styles/index.css";
  import { installIPlugBridge } from "./app/iplugBridge.ts";

  // Initialize iPlug2 bridge before rendering
  installIPlugBridge();

  createRoot(document.getElementById("root")!).render(<App />);
