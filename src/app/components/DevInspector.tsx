import { useState, useEffect, useCallback } from 'react';

const BRIDGE_URL = 'http://localhost:5123';

/**
 * DevInspector — click-to-code overlay
 *
 * Adds a floating toggle button. When active, clicking any element
 * with a `data-src` attribute opens the file:line in VS Code via a local bridge server.
 * The bridge runs `code --goto` on your machine.
 */
export default function DevInspector() {
  const [enabled, setEnabled] = useState(false);
  const [tooltip, setTooltip] = useState<{ x: number; y: number; text: string } | null>(null);

  // Toggle with Cmd+Shift+I / Ctrl+Shift+I
  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if (e.shiftKey && e.metaKey && e.code === 'KeyI') {
        e.preventDefault();
        setEnabled(prev => !prev);
      }
      if (e.code === 'Escape') {
        setTooltip(null);
      }
    };
    window.addEventListener('keydown', handler);
    return () => window.removeEventListener('keydown', handler);
  }, []);

  const handleClick = useCallback((e: MouseEvent) => {
    if (!enabled) return;
    e.preventDefault();
    e.stopPropagation();
    e.stopImmediatePropagation();

    // Walk up to find data-src
    let target = e.target as HTMLElement | null;
    let src = '';
    while (target && target !== document.body) {
      src = target.getAttribute('data-src') || '';
      if (src) break;
      target = target.parentElement;
    }

    if (src) {
      const [file, line] = src.split(':');

      setTooltip({
        x: e.clientX,
        y: e.clientY - 40,
        text: `${file}:${line}`,
      });

      // Hit our local bridge to open VS Code
      const params = new URLSearchParams({ file, line: line || '1' });
      fetch(`${BRIDGE_URL}/open?${params}`).catch(() => {
        // Bridge not running — try vscode:// URL as fallback
        const fullPath = `/Users/jfachriz/Documents/VST_Plugins/ArcadiaEcho1/${file}`;
        const a = document.createElement('a');
        a.href = `vscode://file/${fullPath}:${line || '1'}`;
        a.target = '_blank';
        a.style.display = 'none';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
      });
    }
  }, [enabled]);

  const handleMouseOver = useCallback((e: MouseEvent) => {
    if (!enabled) return;
    const target = e.target as HTMLElement;
    const cursor = target.closest('[data-src]');
    if (cursor) {
      (cursor as HTMLElement).style.outline = '2px solid #6366f1';
      (cursor as HTMLElement).style.outlineOffset = '2px';
      (cursor as HTMLElement).style.cursor = 'crosshair';
    }
  }, [enabled]);

  const handleMouseOut = useCallback((e: MouseEvent) => {
    const target = e.target as HTMLElement;
    const cursor = target.closest('[data-src]');
    if (cursor) {
      (cursor as HTMLElement).style.outline = '';
      (cursor as HTMLElement).style.outlineOffset = '';
      (cursor as HTMLElement).style.cursor = '';
    }
  }, []);

  useEffect(() => {
    if (!enabled) {
      setTooltip(null);
      document.querySelectorAll('[data-src]').forEach(el => {
        (el as HTMLElement).style.outline = '';
        (el as HTMLElement).style.outlineOffset = '';
        (el as HTMLElement).style.cursor = '';
      });
    }
  }, [enabled]);

  useEffect(() => {
    document.addEventListener('click', handleClick, { capture: true });
    document.addEventListener('mouseover', handleMouseOver);
    document.addEventListener('mouseout', handleMouseOut);
    return () => {
      document.removeEventListener('click', handleClick, { capture: true });
      document.removeEventListener('mouseover', handleMouseOver);
      document.removeEventListener('mouseout', handleMouseOut);
    };
  }, [handleClick, handleMouseOver, handleMouseOut]);

  return (
    <>
      {/* Toggle button */}
      <button
        onClick={() => setEnabled(prev => !prev)}
        title={enabled ? 'Disable inspect (Cmd+Shift+I)' : 'Enable inspect (Cmd+Shift+I)'}
        style={{
          position: 'fixed',
          bottom: 20,
          right: 20,
          zIndex: 99999,
          width: 40,
          height: 40,
          borderRadius: 8,
          border: 'none',
          cursor: 'pointer',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          fontSize: 18,
          boxShadow: '0 4px 12px rgba(0,0,0,0.3)',
          transition: 'all 0.2s',
          background: enabled ? '#6366f1' : '#1e1e1e',
          color: '#fff',
          opacity: 0.85,
        }}
        onMouseEnter={e => (e.currentTarget.style.opacity = '1')}
        onMouseLeave={e => (e.currentTarget.style.opacity = '0.85')}
      >
        {enabled ? '✕' : '</>'}
      </button>

      {/* Status bar indicator */}
      {enabled && (
        <div
          style={{
            position: 'fixed',
            bottom: 20,
            right: 68,
            zIndex: 99999,
            background: '#6366f1',
            color: '#fff',
            padding: '6px 14px',
            borderRadius: 6,
            fontSize: 12,
            fontFamily: 'monospace',
            fontWeight: 600,
            boxShadow: '0 4px 12px rgba(0,0,0,0.25)',
            pointerEvents: 'none',
          }}
        >
          Inspect mode — click any element
        </div>
      )}

      {/* Source tooltip */}
      {tooltip && (
        <div
          style={{
            position: 'fixed',
            left: tooltip.x,
            top: tooltip.y,
            zIndex: 99999,
            background: '#1e1e1e',
            color: '#a5b4fc',
            padding: '4px 10px',
            borderRadius: 4,
            fontSize: 11,
            fontFamily: 'monospace',
            boxShadow: '0 4px 12px rgba(0,0,0,0.3)',
            pointerEvents: 'none',
            transform: 'translateX(-50%)',
            whiteSpace: 'nowrap',
          }}
        >
          {tooltip.text}
        </div>
      )}
    </>
  );
}
