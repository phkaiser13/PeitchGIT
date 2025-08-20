import React, { useEffect, useRef } from 'react';

declare const lucide: any;

interface IconProps {
  name: string;
  className?: string;
}

export const Icon: React.FC<IconProps> = ({ name, className }) => {
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (ref.current && typeof lucide !== 'undefined') {
      const iconNode = lucide.createIcons({
        nodes: [ref.current],
        attrs: {},
      });
    }
  }, [name]);

  return (
    <i
      ref={ref as any}
      data-lucide={name}
      className={className}
    ></i>
  );
};
